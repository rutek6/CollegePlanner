#include "solver.h"

#include "preferences.h"
#include <algorithm>
#include <cstdint>

using namespace std;

bool group_fits_prefs(int group_index,
                      int sub_course_index,
                      const vector<sub_course> &sub_courses,
                      const prefs &preferences) {

    for (slot s : sub_courses[sub_course_index].groups[group_index].slots) {
        for (int i = 0; i < 5; i++) {
            if (s.day == i && (s.start < get<0>(preferences.hours[i]) ||
                               s.end > get<1>(preferences.hours[i]))) {
                return false;
            }
            if (s.day == i && preferences.free_days[i] == true) {
                return false;
            }
        }
    }
    return true;
}

void dfs(plan current_plan,
         plan &result,
         size_t &result_conflicts,
         const vector<sub_course> &sub_courses,
         const vector<vector<int>> &allowed,
         const prefs &preferences,
         int index,
         int end) {

    // if (result_conflicts == 0) {
    //     return;
    // }
    if (index == end) {
        size_t conflicts = current_plan.calculate_conflicts(sub_courses);
        if (conflicts < result_conflicts) {
            result = current_plan;
            result.conflicts = conflicts;
            result_conflicts = conflicts;
        }
        return;
    }

    for (int group_index : allowed[index]) {
        if (!group_fits_prefs(group_index, index, sub_courses, preferences)) {
            continue;
        }
        current_plan.chosen.push_back(group_index);
        size_t current_conflicts =
            current_plan.calculate_conflicts(sub_courses);

        if (current_conflicts >= result_conflicts) {
            current_plan.chosen.pop_back();
            continue;
        }

        dfs(current_plan, result, result_conflicts, sub_courses, allowed,
            preferences, index + 1, end);

        current_plan.chosen.pop_back();
    }
}

plan generate_plans(vector<course> selected_courses, prefs preferences) {
    plan result;
    size_t result_conflicts = SIZE_MAX; // każdy pełny plan będzie lepszy

    // sub_courses - wszystkie pod-przedmioty po kolei, bez filtrowania,
    // żeby indeksy w plan::chosen odpowiadały indeksom w selected_courses.
    // allowed[k] - indeksy grup k-tego pod-przedmiotu, które dfs rozważa.
    vector<sub_course> sub_courses;
    vector<vector<int>> allowed;
    int id = 0;

    for (const course &c : selected_courses) {
        for (const sub_course &sub : c.sub_courses) {
            vector<int> required;
            vector<int> all;
            for (int i = 0; i < (int)sub.groups.size(); i++) {
                if (find(preferences.required_group_ids.begin(),
                         preferences.required_group_ids.end(),
                         id) != preferences.required_group_ids.end()) {
                    required.push_back(i);
                }
                all.push_back(i);
                id++;
            }

            sub_courses.push_back(sub);
            allowed.push_back(required.empty() ? all : required);
        }
    }

    plan new_plan;
    dfs(new_plan, result, result_conflicts, sub_courses, allowed, preferences,
        0, sub_courses.size());
    return result;
}
