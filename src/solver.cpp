#include "solver.h"

#include <algorithm>
#include <cstdint>
#include "conflict.h"
#include "preferences.h"

using namespace std;

bool group_fits_prefs(const group& g, const prefs& preferences) {
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

int time_cost(const plan& p, const catalog& cat, const prefs& preferences) {
    int total_time_spent = 0;
    int late_end_penalty = 0;
    int early_start_penalty = 0;
    for (int day = 0; day < day_count; day++) {
        int min_start = 24 * 60, max_end = 0;

        for (size_t i = 0; i < p.chosen.size(); i++) {
            const group& g = cat.groups[p.chosen[i]];
            for (const slot& s : g.slots) {
                if (s.day == day) {
                    if (s.start < min_start) {
                        min_start = s.start;
                    }
                    if (s.end > max_end) {
                        max_end = s.end;
                    }
                }
            }
        }
        if (max_end <= min_start) {
            continue;  // dzień bez zajęć nic nie kosztuje
        }
        total_time_spent += max_end - min_start;
        late_end_penalty += max_end <= 16 * 60 ? max_end : 2 * max_end;

        early_start_penalty += 12 * 60 - min_start;
        early_start_penalty =
            early_start_penalty < 0 ? 0 : 2 * early_start_penalty;
    }

    return preferences.weights_map.at(TIME_SPENT) * total_time_spent +
           preferences.weights_map.at(LATE_END) * late_end_penalty +
           preferences.weights_map.at(EARLY_START) * early_start_penalty;
}

size_t calculate_penalty(plan& current_plan, const prefs& preferences,
                         const vector<vector<CONFLICT_TYPE>>& conflicts,
                         const catalog& cat) {
    size_t nr_of_conflicts = current_plan.calculate_conflicts(conflicts);
    size_t nr_of_soft_conflicts =
        current_plan.calculate_soft_conflicts(conflicts);
    int current_time_cost = time_cost(current_plan, cat, preferences);

    size_t result =
        nr_of_conflicts * preferences.weights_map.at(HARD_CONFLICT) +
        nr_of_soft_conflicts * preferences.weights_map.at(SOFT_CONFLICT) +
        current_time_cost * preferences.weights_map.at(TIME);
    return result;
}

void dfs(plan current_plan, vector<plan>& contenders, size_t& result_penalty,
         const vector<vector<CONFLICT_TYPE>>& conflicts, const catalog& cat,
         const vector<vector<int>>& allowed, const prefs& preferences,
         int index, int end) {

    // if (result_conflicts == 0) {
    //     return;
    // }
    if (index == end) {
        size_t penalty =
            calculate_penalty(current_plan, preferences, conflicts, cat);

        if (penalty < result_penalty) {
            result_penalty = penalty;
            contenders.clear();
            contenders.push_back(current_plan);
        } else if (penalty == result_penalty) {
            contenders.push_back(current_plan);
        }
        return;
    }

    for (int group_id : allowed[index]) {
        if (!group_fits_prefs(cat.groups[group_id], preferences)) {
            continue;
        }
        current_plan.chosen.push_back(group_id);

        if (calculate_penalty(current_plan, preferences, conflicts, cat) >
            result_penalty) {
            current_plan.chosen.pop_back();
            continue;
        }

        dfs(current_plan, contenders, result_penalty, conflicts, cat, allowed,
            preferences, index + 1, end);

        current_plan.chosen.pop_back();
    }
}

vector<plan> generate_plans(const catalog& cat, prefs preferences,
                            const vector<vector<CONFLICT_TYPE>>& conflicts) {
    vector<plan> contenders;           // wszystkie równoważne najlepsze plany
    size_t result_penalty = SIZE_MAX;  // każdy pełny plan będzie lepszy

    // allowed[k] - id grup pod-przedmiotu k, które dfs rozważa: przypięte,
    // a jeśli w tym pod-przedmiocie nic nie przypięto - wszystkie.
    vector<vector<int>> allowed;

    for (const sub_course& sub : cat.sub_courses) {
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
    dfs(new_plan, contenders, result_penalty, conflicts, cat, allowed,
        preferences, 0, cat.sub_courses.size());
    return contenders;
}
