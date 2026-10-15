#include "tui.h"

#include <algorithm>
#include <deque>
#include <fstream>
#include <functional>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include <ftxui/component/app.hpp>
#include <ftxui/component/component.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/terminal.hpp>

#include "conflict.h"
#include "input_text.h"
#include "input_usos.h"
#include "output.h"
#include "solver.h"

using namespace std;
using namespace ftxui;

// Jeden termin wybranej grupy w siatce tygodnia.
struct grid_entry {
    int group_id;
    slot s;
    bool conflict; // nakłada się z innym terminem planu
};

// Ciemne tła typów zajęć (indeksowane enum type) pod białym tekstem - każde
// ma kontrast z bielą co najmniej 7:1. Konflikty zawsze na ciemnej czerwieni.
const Color type_colors[] = {
    Color::RGB(0x16, 0x65, 0x34), // CW    - zielony
    Color::RGB(0x5B, 0x21, 0xB6), // LAB   - fioletowy
    Color::RGB(0x1E, 0x3A, 0x8A), // WYK   - granatowy
    Color::RGB(0x11, 0x5E, 0x59), // CWW   - morski
    Color::RGB(0x78, 0x35, 0x0F), // WF    - brązowy
    Color::RGB(0x70, 0x1A, 0x75), // SEM   - śliwkowy
    Color::RGB(0x33, 0x41, 0x55), // OTHER - grafitowy
};
static_assert(sizeof(type_colors) / sizeof(type_colors[0]) == OTHER + 1,
              "każdy typ zajęć musi mieć swój kolor");
const Color conflict_color = Color::RGB(0x99, 0x1B, 0x1B);

// Wiersze i kolumny zajmowane przez ramkę, nagłówek, stopkę, nazwy dni
// i oś godzin - reszta terminala jest na siatkę.
const int reserved_rows = 8;
const int time_axis_width = 6;
const int reserved_columns = 2 + time_axis_width;

// Najmniejsza liczba minut na wiersz, przy której siatka mieści się
// w available_rows wierszach. Tylko dzielniki 60, żeby pełne godziny
// wypadały na początku wiersza (etykiety osi).
int choose_minutes_per_row(int minutes, int available_rows) {
    for (int minutes_per_row : {5, 10, 15, 20, 30, 60}) {
        if (minutes / minutes_per_row <= available_rows) {
            return minutes_per_row;
        }
    }
    return 60;
}

// Minuta dnia -> numer wiersza siatki (zaokrąglony do najbliższego).
int to_row(int minute, int first_minute, int minutes_per_row) {
    return (minute - first_minute + minutes_per_row / 2) / minutes_per_row;
}

Element class_block(const grid_entry &e, const catalog &cat, int height) {
    const group &g = cat.groups[e.group_id];
    const sub_course &sub = cat.sub_courses[g.sub_id];
    const course &c = cat.courses[sub.course_id];

    // Tyle linii, ile mieści się w wysokości bloku.
    vector<string> lines = {
        c.name,
        string(type_names[sub.class_type]) + " gr. " + to_string(g.nr),
        format_time(e.s.start) + "-" + format_time(e.s.end),
    };
    Elements content;
    for (int i = 0; i < height && i < (int)lines.size(); i++) {
        content.push_back(text(lines[i]));
    }

    Color background =
        e.conflict ? conflict_color : type_colors[sub.class_type];
    Element block = vbox(content) | size(HEIGHT, EQUAL, height) |
                    bgcolor(background) | color(Color::White);
    if (e.conflict) {
        block = block | bold;
    }
    return block;
}

Element empty_rows(int height) {
    return emptyElement() | size(HEIGHT, EQUAL, height);
}

Element day_column(int day,
                   vector<grid_entry> entries,
                   const catalog &cat,
                   int first_minute,
                   int rows,
                   int minutes_per_row,
                   int width) {
    sort(entries.begin(),
         entries.end(),
         [](const grid_entry &a, const grid_entry &b) {
             return a.s.start < b.s.start;
         });

    // Terminy dzielimy na grupy nachodzących na siebie. Pojedynczy termin
    // zajmuje całą szerokość dnia; nakładające się (konflikt) dzielą ją na
    // tory - termin trafia do pierwszego toru, w którym poprzednie zajęcia
    // już się skończyły.
    Elements cells;
    int row = 0;
    size_t i = 0;
    while (i < entries.size()) {
        // Grupa [i, j): terminy zaczynające się przed końcem wcześniejszych.
        size_t j = i + 1;
        int group_end = entries[i].s.end;
        while (j < entries.size() && entries[j].s.start < group_end) {
            group_end = max(group_end, entries[j].s.end);
            j++;
        }

        vector<vector<grid_entry>> lanes;
        vector<int> lane_end;
        for (size_t k = i; k < j; k++) {
            size_t lane = 0;
            while (lane < lanes.size() && lane_end[lane] > entries[k].s.start) {
                lane++;
            }
            if (lane == lanes.size()) {
                lanes.push_back({});
                lane_end.push_back(0);
            }
            lanes[lane].push_back(entries[k]);
            lane_end[lane] = entries[k].s.end;
        }

        // Każdy tor układamy od begin_row; grupa kończy się z najdłuższym.
        int begin_row =
            max(to_row(entries[i].s.start, first_minute, minutes_per_row), row);
        int end_row = begin_row + 1;
        vector<Elements> lane_cells(lanes.size());
        vector<int> lane_row(lanes.size(), begin_row);
        for (size_t l = 0; l < lanes.size(); l++) {
            for (const grid_entry &e : lanes[l]) {
                int start =
                    max(to_row(e.s.start, first_minute, minutes_per_row),
                        lane_row[l]);
                int end = max(to_row(e.s.end, first_minute, minutes_per_row),
                              start + 1);
                if (start > lane_row[l]) {
                    lane_cells[l].push_back(empty_rows(start - lane_row[l]));
                }
                lane_cells[l].push_back(class_block(e, cat, end - start));
                lane_row[l] = end;
            }
            end_row = max(end_row, lane_row[l]);
        }

        // Tory po równo, między nimi odstęp szerokości 1.
        int lane_count = lanes.size();
        int lanes_width = width - (lane_count - 1);
        Elements lane_elements;
        for (int l = 0; l < lane_count; l++) {
            if (lane_row[l] < end_row) {
                lane_cells[l].push_back(empty_rows(end_row - lane_row[l]));
            }
            if (l > 0) {
                lane_elements.push_back(text(" "));
            }
            int lane_width = lanes_width / lane_count +
                             (l < lanes_width % lane_count ? 1 : 0);
            lane_elements.push_back(vbox(lane_cells[l]) |
                                    size(WIDTH, EQUAL, lane_width));
        }

        if (begin_row > row) {
            cells.push_back(empty_rows(begin_row - row));
        }
        cells.push_back(hbox(lane_elements));
        row = end_row;
        i = j;
    }
    if (row < rows) {
        cells.push_back(empty_rows(rows - row));
    }

    return vbox({
               text(day_names[day]) | bold | center,
               separator(),
               vbox(cells),
           }) |
           size(WIDTH, EQUAL, width);
}

Element time_axis(int first_minute, int rows, int minutes_per_row) {
    // Dwie pierwsze linie wyrównują oś z nagłówkami dni (nazwa + separator).
    Elements cells = {text(""), separator()};
    for (int i = 0; i < rows; i++) {
        int minute = first_minute + i * minutes_per_row;
        cells.push_back(
            text(minute % 60 == 0 ? format_time(minute) + " " : ""));
    }
    return vbox(cells) | size(WIDTH, EQUAL, time_axis_width);
}

Element week_grid(const plan &p, const catalog &cat) {
    if (p.chosen.empty()) {
        return text("Nie znaleziono planu spełniającego preferencje") | center;
    }

    vector<grid_entry> entries;
    for (int group_id : p.chosen) {
        for (const slot &s : cat.groups[group_id].slots) {
            entries.push_back({group_id, s, false});
        }
    }
    for (size_t i = 0; i < entries.size(); i++) {
        for (size_t j = i + 1; j < entries.size(); j++) {
            if (entries[i].s.is_conflict(entries[j].s)) {
                entries[i].conflict = true;
                entries[j].conflict = true;
            }
        }
    }

    // Zakres godzin: od pełnej godziny przed pierwszymi zajęciami do pełnej
    // godziny po ostatnich.
    int first_minute = 24 * 60;
    int last_minute = 0;
    int last_day = 4; // Pn-Pt zawsze, So/Nd tylko gdy są w nich zajęcia
    for (const grid_entry &e : entries) {
        first_minute = min(first_minute, e.s.start);
        last_minute = max(last_minute, e.s.end);
        last_day = max(last_day, e.s.day);
    }
    first_minute = first_minute / 60 * 60;
    last_minute = (last_minute + 59) / 60 * 60;

    Dimensions terminal = Terminal::Size();
    int minutes_per_row = choose_minutes_per_row(last_minute - first_minute,
                                                 terminal.dimy - reserved_rows);
    int rows = (last_minute - first_minute) / minutes_per_row;
    // Dni po równo (każdy razem ze swoim separatorem); reszta z dzielenia
    // po jednej kolumnie dla pierwszych dni, żeby siatka sięgała ramki.
    int day_count_shown = last_day + 1;
    int grid_width = terminal.dimx - reserved_columns;
    int day_width = grid_width / day_count_shown - 1;
    int wider_days = grid_width % day_count_shown;

    Elements columns = {time_axis(first_minute, rows, minutes_per_row)};
    for (int day = 0; day <= last_day; day++) {
        vector<grid_entry> day_entries;
        for (const grid_entry &e : entries) {
            if (e.s.day == day) {
                day_entries.push_back(e);
            }
        }
        columns.push_back(separator());
        int width = day_width + (day < wider_days ? 1 : 0);
        columns.push_back(day_column(
            day, day_entries, cat, first_minute, rows, minutes_per_row, width));
    }
    return hbox(columns);
}

// Legenda kolorów: typy zajęć występujące w planie (+ konflikt, jeśli jest).
Element legend(const plan &p, const catalog &cat) {
    bool used[OTHER + 1] = {};
    for (int group_id : p.chosen) {
        used[cat.sub_courses[cat.groups[group_id].sub_id].class_type] = true;
    }
    Elements chips;
    for (int t = 0; t <= OTHER; t++) {
        if (used[t]) {
            chips.push_back(text(string(" ") + type_names[t] + " ") |
                            bgcolor(type_colors[t]) | color(Color::White));
            chips.push_back(text(" "));
        }
    }
    if (p.conflicts > 0) {
        chips.push_back(text(" konflikt ") | bgcolor(conflict_color) |
                        color(Color::White) | bold);
        chips.push_back(text(" "));
    }
    return hbox(chips);
}

// ---------------------------------------------------------------------------
// Panel preferencji
// ---------------------------------------------------------------------------

// Preferencje w postaci, którą edytują komponenty FTXUI: Checkbox potrzebuje
// bool*, Input - string. Zmiany trafiają do prefs dopiero po zapisie.
struct prefs_editor {
    bool free_days[5];
    string start[5]; // "HH:MM"
    string end[5];
    deque<bool> pinned; // pinned[id grupy]; deque nie przenosi elementów,
                        // więc wskaźniki dane Checkboxom zostają ważne
    string path;        // plik, do którego zapisujemy
    string error;       // komunikat błędu pod przyciskami
};

void load_editor(prefs_editor &editor, const prefs &preferences) {
    for (int day = 0; day < 5; day++) {
        editor.free_days[day] = preferences.free_days[day];
        editor.start[day] = format_time(get<0>(preferences.hours[day]));
        editor.end[day] = format_time(get<1>(preferences.hours[day]));
    }
    for (size_t id = 0; id < editor.pinned.size(); id++) {
        editor.pinned[id] = false;
    }
    for (int group_id : preferences.required_group_ids) {
        editor.pinned[group_id] = true;
    }
    editor.error.clear();
}

// Przepisuje stan edytora do result. Przy błędnej godzinie zwraca false
// i ustawia editor.error.
bool editor_to_prefs(prefs_editor &editor, prefs &result) {
    for (int day = 0; day < 5; day++) {
        int start, end;
        if (!parse_time_range(
                editor.start[day] + "-" + editor.end[day], start, end)) {
            editor.error = string(day_names[day]) +
                           ": niepoprawna godzina (oczekiwano HH:MM)";
            return false;
        }
        if (start >= end) {
            editor.error =
                string(day_names[day]) + ": początek musi być przed końcem";
            return false;
        }
        result.hours[day] = make_tuple(start, end);
        result.free_days[day] = editor.free_days[day];
    }
    result.required_group_ids.clear();
    for (size_t id = 0; id < editor.pinned.size(); id++) {
        if (editor.pinned[id]) {
            result.required_group_ids.push_back(id);
        }
    }
    return true;
}

// "5 (Pn 12:15, Cz 10:15)" - numer grupy i początki jej zajęć.
string group_label(const group &g) {
    string label = to_string(g.nr) + " (";
    for (size_t i = 0; i < g.slots.size(); i++) {
        if (i > 0) {
            label += ", ";
        }
        label += string(day_names[g.slots[i].day]) + " " +
                 format_time(g.slots[i].start);
    }
    return label + ")";
}

Component time_input(string *content) {
    InputOption option;
    option.multiline = false;
    Component input = Input(content, "HH:MM", option);
    return Renderer(input, [input] {
        return hbox(
            {text("["), input->Render() | size(WIDTH, EQUAL, 5), text("]")});
    });
}

Component build_prefs_panel(prefs_editor &editor,
                            const catalog &cat,
                            function<void()> on_save,
                            function<void()> on_cancel) {
    // Dni: wolny dzień i zakres godzin, w którym mogą być zajęcia.
    Components day_rows;
    for (int day = 0; day < 5; day++) {
        Component free_day = Checkbox("wolny", &editor.free_days[day]);
        Component start = time_input(&editor.start[day]);
        Component end = time_input(&editor.end[day]);
        Component row = Container::Horizontal({free_day, start, end});
        day_rows.push_back(Renderer(row, [=] {
            return hbox(
                {text(string(" ") + day_names[day]) | size(WIDTH, EQUAL, 4),
                 free_day->Render(),
                 text("   od "),
                 start->Render(),
                 text(" do "),
                 end->Render()});
        }));
    }
    Component days = Container::Vertical(day_rows);

    // Przypięcia: dla każdych zajęć pola wyboru przy wszystkich grupach.
    // W kontenerze są tylko wiersze z polami (nagłówki przedmiotów nie mogą
    // dostawać fokusu), nagłówki dorysowujemy przy renderowaniu.
    Components pin_rows;
    vector<string> header_before; // nazwa przedmiotu przed wierszem albo ""
    for (const course &c : cat.courses) {
        for (int sub_id : c.sub_ids) {
            const sub_course &sub = cat.sub_courses[sub_id];
            Components boxes;
            for (int group_id : sub.group_ids) {
                boxes.push_back(Checkbox(group_label(cat.groups[group_id]),
                                         &editor.pinned[group_id]));
            }
            Component row = Container::Horizontal(boxes);
            string type_label = string("   ") + type_names[sub.class_type];
            pin_rows.push_back(Renderer(row, [=] {
                Elements rendered;
                for (const Component &box : boxes) {
                    rendered.push_back(hbox({box->Render(), text("  ")}));
                }
                return hbox({text(type_label) | size(WIDTH, EQUAL, 9),
                             hflow(rendered) | flex});
            }));
            header_before.push_back(sub_id == c.sub_ids.front() ? c.name : "");
        }
    }
    Component pins_container = Container::Vertical(pin_rows);
    Component pins = Renderer(pins_container, [=] {
        Elements lines;
        for (size_t i = 0; i < pin_rows.size(); i++) {
            if (!header_before[i].empty()) {
                lines.push_back(text(" " + header_before[i]) | bold);
            }
            lines.push_back(pin_rows[i]->Render());
        }
        return vbox(lines);
    });

    InputOption path_option;
    path_option.multiline = false;
    Component path = Input(&editor.path, "plik.in", path_option);
    Component save = Button(" Zapisz i przelicz ", on_save);
    Component cancel = Button(" Anuluj ", on_cancel);
    Component buttons = Container::Horizontal({save, cancel});

    Component panel = Container::Vertical({days, pins, path, buttons});
    return Renderer(panel, [=, &editor] {
        return vbox({
            text(" Dni (zajęcia muszą się mieścić w podanych godzinach)") |
                bold,
            days->Render(),
            separator(),
            text(" Przypięte grupy - w zajęciach z zaznaczeniem solver wybiera "
                 "tylko spośród zaznaczonych") |
                bold,
            pins->Render() | vscroll_indicator | yframe | flex,
            separator(),
            hbox({text(" Plik: "), path->Render() | flex}),
            hbox({text(" "),
                  save->Render(),
                  text(" "),
                  cancel->Render(),
                  text("  "),
                  text(editor.error) | color(Color::Red) | bold}),
        });
    });
}

// ---------------------------------------------------------------------------
// Przenoszenie przypięć między danymi
// ---------------------------------------------------------------------------

// Grupa opisana tak, jak widzi ją użytkownik - po wczytaniu nowego pliku
// id grup są inne, ale nazwa przedmiotu, typ i numer zostają.
struct group_key {
    string course_name;
    type class_type;
    int nr;
};

vector<group_key> keys_of(const catalog &cat, const vector<int> &group_ids) {
    vector<group_key> keys;
    for (int group_id : group_ids) {
        const group &g = cat.groups[group_id];
        const sub_course &sub = cat.sub_courses[g.sub_id];
        keys.push_back({cat.courses[sub.course_id].name, sub.class_type, g.nr});
    }
    return keys;
}

// id grup z keys w cat; grup, których nie ma, po prostu brak w wyniku.
vector<int> ids_of(const catalog &cat, const vector<group_key> &keys) {
    vector<int> ids;
    for (const group_key &key : keys) {
        for (size_t id = 0; id < cat.groups.size(); id++) {
            const group &g = cat.groups[id];
            const sub_course &sub = cat.sub_courses[g.sub_id];
            if (g.nr == key.nr && sub.class_type == key.class_type &&
                cat.courses[sub.course_id].name == key.course_name) {
                ids.push_back(id);
                break;
            }
        }
    }
    return ids;
}

// ---------------------------------------------------------------------------

void run_tui(string data_path,
             catalog cat,
             vector<vector<CONFLICT_TYPE>> conflicts,
             prefs &preferences,
             vector<plan> plans,
             const string &prefs_path) {
    App app = App::Fullscreen();

    const plan no_plan; // pusty plan - week_grid pokaże komunikat
    int current = 0;    // numer oglądanego planu (od 0)
    int mode = 0;       // 0 - plan, 1 - panel preferencji, 2 - otwieranie pliku
    string status;      // komunikat w nagłówku, np. po zapisie

    // Edytor i panel preferencji zależą od danych (pola przy grupach), więc
    // po wczytaniu nowego pliku budujemy je od nowa - stąd unique_ptr.
    auto editor = make_unique<prefs_editor>();
    editor->pinned.resize(cat.groups.size());
    editor->path = prefs_path;

    string open_path = data_path; // pole w oknie "Otwórz plik"
    string open_error;

    // Obliczenia (przeliczanie planów, wczytywanie pliku) robi osobny wątek,
    // żeby ekran mógł w tym czasie pokazać komunikat. Wynik wraca przez
    // app.Post - zadanie wykonuje się w wątku interfejsu, więc tylko on
    // zmienia cat, conflicts i plans. Póki computing == true, nowych obliczeń
    // nie zaczynamy, więc wątek bezpiecznie czyta cat i conflicts.
    thread worker;
    bool computing = false;
    auto start_worker = [&](function<void()> task) {
        computing = true;
        if (worker.joinable()) {
            worker.join(); // poprzedni wątek już skończył (computing == false)
        }
        worker = thread([&app, task] {
            task();
            app.PostEvent(Event::Custom); // samo Post nie odświeża ekranu
        });
    };

    auto close_panel = [&] { mode = 0; };
    auto save = [&] {
        if (computing) {
            editor->error = "poprzednie obliczenia jeszcze trwają";
            return;
        }
        prefs updated = preferences;
        if (!editor_to_prefs(*editor, updated)) {
            return;
        }
        ofstream file(editor->path);
        if (!file) {
            editor->error = "nie można zapisać pliku \"" + editor->path + "\"";
            return;
        }
        write_prefs(updated, cat, file);
        file.close();

        preferences = updated;
        mode = 0;
        status = "przeliczanie...";
        start_worker([&, updated, path = editor->path] {
            vector<plan> result = generate_plans(cat, updated, conflicts);
            app.Post([&, result, path] {
                plans = result;
                current = 0;
                status = "zapisano w " + path;
                computing = false;
            });
        });
    };

    Component panel_slot = Container::Vertical({});
    auto rebuild_panel = [&] {
        string path = editor->path;
        panel_slot->DetachAllChildren(); // najpierw komponenty, potem edytor
        editor = make_unique<prefs_editor>();
        editor->pinned.resize(cat.groups.size());
        editor->path = path;
        panel_slot->Add(build_prefs_panel(*editor, cat, save, close_panel));
    };
    rebuild_panel();

    auto open_file = [&] {
        if (computing) {
            open_error = "poprzednie obliczenia jeszcze trwają";
            return;
        }
        mode = 0;
        status = "wczytywanie " + open_path + "...";
        prefs base = preferences;
        vector<group_key> pinned = keys_of(cat, preferences.required_group_ids);
        start_worker([&, base, pinned, path = open_path]() mutable {
            try {
                string warnings;
                catalog new_cat = load_catalog(path, warnings);
                base.required_group_ids = ids_of(new_cat, pinned);
                vector<vector<CONFLICT_TYPE>> new_conflicts =
                    create_conflict_graph(new_cat);
                vector<plan> new_plans =
                    generate_plans(new_cat, base, new_conflicts);
                int warning_count =
                    count(warnings.begin(), warnings.end(), '\n');
                int lost_pins = pinned.size() - base.required_group_ids.size();
                app.Post([&,
                          new_cat,
                          new_conflicts,
                          new_plans,
                          base,
                          path,
                          warning_count,
                          lost_pins] {
                    cat = new_cat;
                    conflicts = new_conflicts;
                    plans = new_plans;
                    preferences = base;
                    current = 0;
                    data_path = path;
                    mode = 0;
                    rebuild_panel();
                    status = "wczytano " + path;
                    if (warning_count > 0) {
                        status += ", ostrzeżenia parsera: " +
                                  to_string(warning_count);
                    }
                    if (lost_pins > 0) {
                        status += ", odpięte grupy (brak w danych): " +
                                  to_string(lost_pins);
                    }
                    computing = false;
                });
            } catch (const input_exception &e) {
                string message = e.what();
                app.Post([&, message] {
                    status = message;
                    computing = false;
                });
            }
        });
    };

    // Renderer jest wywoływany przy każdym odświeżeniu ekranu (także po
    // zmianie rozmiaru terminala i po przełączeniu planu), więc rysuje zawsze
    // aktualny plan dopasowany do okna.
    Component plan_view = Renderer([&] {
        int plan_count = plans.size();
        const plan &p = plan_count > 0 ? plans[current] : no_plan;

        Element conflicts_info =
            text("Konflikty: " + to_string(p.conflicts) + " ");
        if (p.chosen.empty()) {
            conflicts_info = text(""); // brak planu - liczba nic nie znaczy
        }
        else if (p.conflicts > 0) {
            conflicts_info = conflicts_info | color(Color::Red) | bold;
        }

        Element plan_number = text("");
        string keys = " o - otwórz plik   p - preferencje   q - wyjście";
        if (plan_count > 1) {
            plan_number = text("Plan " + to_string(current + 1) + "/" +
                               to_string(plan_count)) |
                          bold;
            keys =
                " ←/→ - plany   Home/End - pierwszy/ostatni   o - otwórz plik"
                "   p - preferencje   q - wyjście";
        }

        return vbox({
                   hbox({text(" CollegePlanner") | bold,
                         text(" - " + data_path),
                         text(status.empty() ? "" : "  (" + status + ")") | dim,
                         filler(),
                         plan_number,
                         filler(),
                         conflicts_info}),
                   separator(),
                   week_grid(p, cat) | flex,
                   separator(),
                   hbox({text(keys) | dim, filler(), legend(p, cat)}),
               }) |
               border;
    });

    Component prefs_view = Renderer(panel_slot, [&] {
        return vbox({
                   hbox({text(" CollegePlanner - preferencje") | bold,
                         filler(),
                         text("strzałki - poruszanie   Spacja/Enter - "
                              "zaznacz   Esc - anuluj ") |
                             dim}),
                   separator(),
                   panel_slot->Render() | flex,
               }) |
               border;
    });

    InputOption open_option;
    open_option.multiline = false;
    open_option.on_enter = open_file;
    Component open_input =
        Input(&open_path, "plan.html albo dane.in", open_option);
    Component open_button = Button(" Wczytaj ", open_file);
    Component open_cancel = Button(" Anuluj ", close_panel);
    Component open_form = Container::Vertical(
        {open_input, Container::Horizontal({open_button, open_cancel})});
    Component open_view = Renderer(open_form, [&] {
        return vbox({
                   hbox({text(" CollegePlanner - otwórz plik") | bold,
                         filler(),
                         text("Enter - wczytaj   Esc - anuluj ") | dim}),
                   separator(),
                   text(" Plan z USOSweb (.html) przejdzie przez parser "
                        "src/usos_to_in.py; inne pliki są czytane jako .in."),
                   text(" Przypięte grupy zostaną przeniesione, jeśli są w "
                        "nowych danych."),
                   text(""),
                   hbox({text(" Plik: "), open_input->Render() | flex}),
                   text(""),
                   hbox({text(" "),
                         open_button->Render(),
                         text(" "),
                         open_cancel->Render(),
                         text("  "),
                         text(open_error) | color(Color::Red) | bold}),
                   filler(),
               }) |
               border;
    });

    Component root = Container::Tab({plan_view, prefs_view, open_view}, &mode);
    root |= CatchEvent([&](Event event) {
        if (mode != 0) {
            // W panelach klawisze trafiają do pól (np. 'q' w nazwie pliku);
            // przechwytujemy tylko Esc.
            if (event == Event::Escape) {
                close_panel();
                return true;
            }
            return false;
        }
        if (event == Event::Character('q') || event == Event::Escape) {
            app.Exit();
            return true;
        }
        if (event == Event::Character('p')) {
            load_editor(*editor, preferences);
            mode = 1;
            return true;
        }
        if (event == Event::Character('o')) {
            open_error.clear();
            mode = 2;
            return true;
        }
        int plan_count = plans.size();
        if (plan_count == 0) {
            return false;
        }
        // Strzałki przechodzą w kółko: za ostatnim planem jest pierwszy.
        if (event == Event::ArrowRight || event == Event::Character('l')) {
            current = (current + 1) % plan_count;
            return true;
        }
        if (event == Event::ArrowLeft || event == Event::Character('h')) {
            current = (current - 1 + plan_count) % plan_count;
            return true;
        }
        if (event == Event::Home) {
            current = 0;
            return true;
        }
        if (event == Event::End) {
            current = plan_count - 1;
            return true;
        }
        return false;
    });

    app.Loop(root);
    // Wątek korzysta z cat i conflicts - musi skończyć, zanim znikną.
    if (worker.joinable()) {
        worker.join();
    }
}
