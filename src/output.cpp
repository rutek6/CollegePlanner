#include "output.h"

#include <cstdio>
#include <iostream>

using namespace std;

string format_time(int minutes) {
    char buffer[16];
    snprintf(buffer, sizeof(buffer), "%02d:%02d", minutes / 60, minutes % 60);
    return buffer;
}

void print_plan(const plan &p, const vector<course> &courses, ostream &out) {
    out << "Liczba konfliktów: " << p.conflicts << "\n\n";

    size_t index = 0;
    for (const course &c : courses) {
        out << c.name << '\n';
        for (const sub_course &sub : c.sub_courses) {
            out << "  " << type_names[sub.class_type] << ": ";
            if (index >= p.chosen.size()) {
                out << "(brak wybranej grupy)\n";
                continue;
            }
            const group &g = sub.groups[p.chosen[index++]];
            out << "grupa " << g.nr;
            for (const slot &s : g.slots) {
                out << " | " << day_names[s.day] << ' ' << format_time(s.start)
                    << '-' << format_time(s.end);
            }
            out << '\n';
        }
    }

    if (index != p.chosen.size()) {
        cerr << "Uwaga: plan zawiera " << p.chosen.size()
             << " grup, a pod-przedmiotów jest " << index << '\n';
    }
}
