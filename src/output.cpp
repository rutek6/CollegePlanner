#include "output.h"
#include "model.h"

#include <algorithm>
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
    // for (const course &c : courses) {
    //     out << c.name << '\n';
    //     for (const sub_course &sub : c.sub_courses) {
    //         out << "  " << type_names[sub.class_type] << ": ";
    //         if (index >= p.chosen.size()) {
    //             out << "(brak wybranej grupy)\n";
    //             continue;
    //         }
    //         const group &g = sub.groups[p.chosen[index++]];
    //         out << "grupa " << g.nr;
    //         for (const slot &s : g.slots) {
    //             out << " | " << day_names[s.day] << ' ' <<
    //             format_time(s.start)
    //                 << '-' << format_time(s.end);
    //         }
    //         out << '\n';
    //     }
    // }
    int course_index = 0;

    vector<tuple<int, int, int, slot>> slots;
    for (const course &c : courses) {
        int sub_course_index = 0;
        for (const sub_course &sub : c.sub_courses) {
            int group_index = 0;
            for (int i = 0; i < sub.groups[p.chosen.at(index)].slots.size();
                 i++) {
                slots.push_back(
                    make_tuple(course_index, sub_course_index, group_index,
                               sub.groups[p.chosen.at(index)].slots[i]));
                group_index++;
            }
            sub_course_index++;
        }
        course_index++;
    }
    sort(slots.begin(), slots.end(),
         [](tuple<int, int, int, slot> &a, tuple<int, int, int, slot> &b) {
             slot slotA = get<3>(a);
             slot slotB = get<3>(b);

             if (slotA.day < slotB.day) {
                 return true;
             }
             else if (slotA.day > slotB.day) {
                 return false;
             }
             else if (slotA.start < slotB.start) {
                 return true;
             }
             else {
                 return false;
             }
         });
    int previous_day = -1;
    for (tuple<int, int, int, slot> entry : slots) {
        int course_index = get<0>(entry);
        int sub_course_index = get<1>(entry);
        int group_index = get<2>(entry);
        slot s = get<3>(entry);
        if (previous_day != s.day) {
            cout << day_names[s.day] << endl;
            previous_day = s.day;
        }

        cout << "\t" << courses[course_index].name << " "
             << type_names[courses[course_index]
                               .sub_courses[sub_course_index]
                               .class_type]
             << " - nr "
             << courses[course_index]
                    .sub_courses[sub_course_index]
                    .groups[group_index]
                    .nr
             << " | " << format_time(s.start) << '-' << format_time(s.end)
             << endl;
    }
}
