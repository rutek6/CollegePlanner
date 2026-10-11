#include "model.h"

using namespace std;

const char *type_names[] = {"CW", "LAB", "WYK", "CWW", "WF", "SEM", "OTHER"};
const int type_count = sizeof(type_names) / sizeof(type_names[0]);
const char *day_names[] = {"Pn", "Wt", "Sr", "Cz", "Pt", "So", "Nd"};
const int day_count = sizeof(day_names) / sizeof(day_names[0]);

bool slot::is_conflict(slot second) const {
    if (day != second.day) {
        return false;
    }
    if (start < second.end && start >= second.start) {
        return true;
    }
    else if (second.start < end && second.start >= start) {
        return true;
    }

    return false;
}

bool group::is_conflict(group second) const {
    for (size_t i = 0; i < slots.size(); i++) {
        for (size_t j = 0; j < second.slots.size(); j++) {
            if (slots[i].is_conflict(second.slots[j])) {
                return true;
            }
        }
    }
    return false;
}

size_t
plan::calculate_conflicts(const std::vector<std::vector<bool>> &conflicts) {
    size_t nr_of_conflicts = 0;
    for (size_t i = 0; i < chosen.size(); i++) {
        for (size_t j = i + 1; j < chosen.size(); j++) {
            if (conflicts[chosen[i]][chosen[j]]) {
                nr_of_conflicts++;
            }
        }
    }
    return nr_of_conflicts;
}
