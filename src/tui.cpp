#include "tui.h"

#include <algorithm>
#include <string>
#include <vector>

#include <ftxui/component/app.hpp>
#include <ftxui/component/component.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/terminal.hpp>

#include "output.h"

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

    Color background = e.conflict
                           ? conflict_color
                           : type_colors[sub.class_type];
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
    sort(entries.begin(), entries.end(),
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
                int start = max(to_row(e.s.start, first_minute, minutes_per_row),
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
            int lane_width =
                lanes_width / lane_count + (l < lanes_width % lane_count ? 1 : 0);
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
        cells.push_back(text(minute % 60 == 0 ? format_time(minute) + " " : ""));
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
        columns.push_back(day_column(day, day_entries, cat, first_minute, rows,
                                     minutes_per_row, width));
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

void run_tui(const catalog &cat, const plan &p) {
    App app = App::Fullscreen();

    // Renderer jest wywoływany przy każdym odświeżeniu ekranu (także po
    // zmianie rozmiaru terminala), więc siatka dopasowuje się do okna.
    Component view = Renderer([&] {
        Element conflicts = text("Konflikty: " + to_string(p.conflicts) + " ");
        if (p.chosen.empty()) {
            conflicts = text(""); // brak planu - liczba nic nie znaczy
        }
        else if (p.conflicts > 0) {
            conflicts = conflicts | color(Color::Red) | bold;
        }
        return vbox({
                   hbox({text(" CollegePlanner") | bold, filler(), conflicts}),
                   separator(),
                   week_grid(p, cat) | flex,
                   separator(),
                   hbox({text(" q - wyjście") | dim, filler(), legend(p, cat)}),
               }) |
               border;
    });

    view |= CatchEvent([&](Event event) {
        if (event == Event::Character('q') || event == Event::Escape) {
            app.Exit();
            return true;
        }
        return false;
    });

    app.Loop(view);
}
