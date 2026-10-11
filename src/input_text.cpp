#include "input_text.h"

#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <sstream>

using namespace std;

[[noreturn]] void input_error(int line_nr, const string &message) {
    cerr << "Błąd danych wejściowych (linia " << line_nr << "): " << message
         << '\n';
    exit(1);
}

string trim(const string &s) {
    size_t first = s.find_first_not_of(" \t\r");
    if (first == string::npos) {
        return "";
    }
    size_t last = s.find_last_not_of(" \t\r");
    return s.substr(first, last - first + 1);
}

int find_name(const char *names[], int count, const string &name) {
    for (int i = 0; i < count; i++) {
        if (name == names[i]) {
            return i;
        }
    }
    return -1;
}

// "08:15-10:00" (dopuszczalne spacje wokół '-') -> minuty od 00:00
bool parse_time_range(const string &s, int &start, int &end) {
    int h1, m1, h2, m2;
    char extra;
    if (sscanf(s.c_str(), " %d:%d - %d:%d %c", &h1, &m1, &h2, &m2, &extra) !=
        4) {
        return false;
    }
    if (h1 < 0 || h1 > 23 || m1 < 0 || m1 > 59 || h2 < 0 || h2 > 24 || m2 < 0 ||
        m2 > 59) {
        return false;
    }
    start = h1 * 60 + m1;
    end = h2 * 60 + m2;
    return true;
}

catalog read_catalog(istream &in) {
    catalog cat;
    string line;
    int line_nr = 0;

    while (getline(in, line)) {
        line_nr++;
        size_t comment = line.find('#');
        if (comment != string::npos) {
            line.erase(comment);
        }
        line = trim(line);
        if (line.empty()) {
            continue;
        }

        istringstream tokens(line);
        string keyword;
        tokens >> keyword;
        string rest;
        getline(tokens, rest);
        rest = trim(rest);

        if (keyword == "PRZEDMIOT") {
            if (rest.empty()) {
                input_error(line_nr, "brak nazwy przedmiotu");
            }
            course c;
            c.name = rest;
            cat.courses.push_back(c);
        }
        else if (keyword == "ZAJECIA") {
            if (cat.courses.empty()) {
                input_error(line_nr, "ZAJECIA przed pierwszym PRZEDMIOTEM");
            }
            int t = find_name(type_names, type_count, rest);
            if (t < 0) {
                input_error(line_nr, "nieznany typ zajęć \"" + rest + "\"");
            }
            sub_course sub;
            sub.class_type = static_cast<type>(t);
            sub.course_id = cat.courses.size() - 1;
            cat.courses.back().sub_ids.push_back(cat.sub_courses.size());
            cat.sub_courses.push_back(sub);
        }
        else if (keyword == "GRUPA") {
            // sub_courses.back() to ostatnie ZAJECIA ostatniego PRZEDMIOTU
            // tylko wtedy, gdy ten przedmiot ma już jakieś ZAJECIA.
            if (cat.courses.empty() || cat.courses.back().sub_ids.empty()) {
                input_error(line_nr, "GRUPA przed pierwszymi ZAJECIAMI");
            }
            group g;
            char extra;
            if (sscanf(rest.c_str(), "%d %c", &g.nr, &extra) != 1) {
                input_error(line_nr,
                            "niepoprawny numer grupy \"" + rest + "\"");
            }
            g.sub_id = cat.sub_courses.size() - 1;
            cat.sub_courses.back().group_ids.push_back(cat.groups.size());
            cat.groups.push_back(g);
        }
        else if (find_name(day_names, day_count, keyword) >= 0) {
            if (cat.courses.empty() || cat.courses.back().sub_ids.empty() ||
                cat.sub_courses.back().group_ids.empty()) {
                input_error(line_nr, "termin zajęć przed pierwszą GRUPĄ");
            }
            slot s;
            s.day = find_name(day_names, day_count, keyword);
            if (!parse_time_range(rest, s.start, s.end)) {
                input_error(line_nr, "niepoprawny przedział czasu \"" + rest +
                                         "\" (oczekiwano HH:MM-HH:MM)");
            }
            if (s.start >= s.end) {
                input_error(line_nr, "początek zajęć nie jest przed końcem");
            }
            cat.groups.back().slots.push_back(s);
        }
        else {
            input_error(line_nr, "nieznane słowo kluczowe \"" + keyword + "\"");
        }
    }

    // Przedmiot bez zajęć albo zajęcia bez grup uniemożliwiają ułożenie planu.
    for (const course &c : cat.courses) {
        if (c.sub_ids.empty()) {
            cerr << "Błąd danych wejściowych: przedmiot \"" << c.name
                 << "\" nie ma żadnych ZAJEC\n";
            exit(1);
        }
        for (int sub_id : c.sub_ids) {
            const sub_course &sub = cat.sub_courses[sub_id];
            if (sub.group_ids.empty()) {
                cerr << "Błąd danych wejściowych: zajęcia "
                     << type_names[sub.class_type] << " przedmiotu \"" << c.name
                     << "\" nie mają żadnej GRUPY\n";
                exit(1);
            }
        }
    }

    return cat;
}

void read_prefs(std::tuple<int, int> *hours,
                bool *free_days,
                std::istream &in) {
    string line;
    int line_nr = 0;
    while (getline(in, line)) {
        line_nr++;
        if (line_nr <= 5) {
            int start, end;
            if (!parse_time_range(line, start, end)) {
                input_error(line_nr, "niepoprawny przedział czasu \"" + line +
                                         "\" (oczekiwano HH:MM-HH:MM)");
            }
            if (start >= end) {
                input_error(line_nr, "początek zajęć nie jest przed końcem");
            }
            get<0>(hours[line_nr - 1]) = start;
            get<1>(hours[line_nr - 1]) = end;
        }
        else {
            line = trim(line);
            if (line == "true") {
                free_days[line_nr - 6] = true;
            }
            else if (line == "false") {
                free_days[line_nr - 6] = false;
            }
            else {
                input_error(line_nr, "niepoprawne dane\"");
            }
        }
    }
}