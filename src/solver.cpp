#include "solver.h"

#include "preferences.h"
#include <algorithm>
#include <cstdint>

using namespace std;

bool group_fits_prefs(const group &g, const prefs &preferences) {
    for (slot s : g.slots) {
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
         const vector<vector<bool>> &conflicts,
         const vector<group> &groups,
         const vector<vector<int>> &allowed,
         const prefs &preferences,
         int index,
         int end) {

    // if (result_conflicts == 0) {
    //     return;
    // }
    if (index == end) {
        size_t nr_of_conflicts = current_plan.calculate_conflicts(conflicts);
        if (nr_of_conflicts < result_conflicts) {
            result = current_plan;
            result.conflicts = nr_of_conflicts;
            result_conflicts = nr_of_conflicts;
        }
        return;
    }

    for (int group_id : allowed[index]) {
        if (!group_fits_prefs(groups[group_id], preferences)) {
            continue;
        }
        current_plan.chosen.push_back(group_id);
        size_t current_conflicts = current_plan.calculate_conflicts(conflicts);

        if (current_conflicts >= result_conflicts) {
            current_plan.chosen.pop_back();
            continue;
        }

        dfs(current_plan, result, result_conflicts, conflicts, groups, allowed,
            preferences, index + 1, end);

        current_plan.chosen.pop_back();
    }
}

plan generate_plans(const catalog &cat,
                    prefs preferences,
                    const vector<vector<bool>> &conflicts) {
    plan result;
    size_t result_conflicts = SIZE_MAX; // każdy pełny plan będzie lepszy

    // allowed[k] - id grup pod-przedmiotu k, które dfs rozważa: przypięte,
    // a jeśli w tym pod-przedmiocie nic nie przypięto - wszystkie.
    vector<vector<int>> allowed;

    for (const sub_course &sub : cat.sub_courses) {
        vector<int> required;
        for (int group_id : sub.group_ids) {
            if (find(preferences.required_group_ids.begin(),
                     preferences.required_group_ids.end(),
                     group_id) != preferences.required_group_ids.end()) {
                required.push_back(group_id);
            }
        }
        allowed.push_back(required.empty() ? sub.group_ids : required);
    }

    plan new_plan;
    dfs(new_plan, result, result_conflicts, conflicts, cat.groups, allowed,
        preferences, 0, cat.sub_courses.size());
    return result;
}
