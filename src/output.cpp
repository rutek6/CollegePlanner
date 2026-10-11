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

void print_plan(const plan &p, const catalog &cat, ostream &out) {
    out << "Liczba konfliktów: " << p.conflicts << "\n\n";

    // (id grupy, termin) - z id grupy da się dojść do wszystkiego innego:
    // group::sub_id -> pod-przedmiot, sub_course::course_id -> przedmiot.
    vector<pair<int, slot>> slots;
    for (int group_id : p.chosen) {
        for (const slot &s : cat.groups[group_id].slots) {
            slots.push_back(make_pair(group_id, s));
        }
    }
    sort(slots.begin(), slots.end(),
         [](const pair<int, slot> &a, const pair<int, slot> &b) {
             slot slotA = a.second;
             slot slotB = b.second;

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
    for (const pair<int, slot> &entry : slots) {
        const group &g = cat.groups[entry.first];
        const sub_course &sub = cat.sub_courses[g.sub_id];
        const course &c = cat.courses[sub.course_id];
        const slot &s = entry.second;
        if (previous_day != s.day) {
            out << day_names[s.day] << endl;
            previous_day = s.day;
        }

        out << "\t" << c.name << " " << type_names[sub.class_type] << " - nr "
            << g.nr << " | " << format_time(s.start) << '-'
            << format_time(s.end) << endl;
    }
}
