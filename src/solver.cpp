#include "solver.h"

#include <algorithm>
#include <cstdint>
#include <iostream>
#include "conflict.h"
#include "model.h"
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

int time_cost(plan& p, const prefs& preferences) {
    int total_time_spent = 0;
    int late_end_penalty = 0;
    int early_start_penalty = 0;

    for (int day = 0; day < day_count; day++) {
        int& min_start = p.params.min_start[day];
        int& max_end = p.params.max_end[day];

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

size_t calculate_penalty(plan& current_plan, int current_group_id,
                         const prefs& preferences,
                         const vector<vector<CONFLICT_TYPE>>& conflicts,
                         const catalog& cat) {

    // Count hard and soft conflicts
    size_t current_soft_conflicts = 0, current_hard_conflicts = 0;
    for (int group_id : current_plan.chosen) {
        if (conflicts[group_id][current_group_id] == HARD) {
            current_hard_conflicts++;
        } else if (conflicts[group_id][current_group_id] == SOFT) {
            current_soft_conflicts++;
        }
    }
    for (slot group_slot : cat.groups[current_group_id].slots) {
        current_plan.params.min_start[group_slot.day] = std::min(
            current_plan.params.min_start[group_slot.day], group_slot.start);
        current_plan.params.max_end[group_slot.day] = std::max(
            current_plan.params.max_end[group_slot.day], group_slot.end);
    }

    size_t hard_conflicts =
        current_plan.params.hard_conflicts + current_hard_conflicts;
    size_t soft_conflicts =
        current_plan.params.soft_conflicts + current_soft_conflicts;
    int current_time_cost = time_cost(current_plan, preferences);

    current_plan.params.hard_conflicts = hard_conflicts;
    current_plan.params.soft_conflicts = soft_conflicts;

    size_t result = hard_conflicts * preferences.weights_map.at(HARD_CONFLICT) +
                    soft_conflicts * preferences.weights_map.at(SOFT_CONFLICT) +
                    current_time_cost * preferences.weights_map.at(TIME);
    current_plan.params.penalty = result;
    return result;
}

size_t find_penalty_limit(const vector<vector<int>>& groups, const catalog& cat,
                          const prefs& preferences,
                          const vector<vector<CONFLICT_TYPE>>& conflicts) {
    plan result;
    for (size_t i = 0; i < groups.size(); i++) {
        plan_parameters current_params = result.params;
        vector<int> group_ids = groups[i];
        size_t min_penalty = SIZE_MAX;
        plan_parameters best_params;
        int best_group_id;
        for (int j = 0; j < group_ids.size(); j++) {
            if (calculate_penalty(result, group_ids[j], preferences, conflicts,
                                  cat) < min_penalty) {
                best_params = result.params;
                best_group_id = group_ids[j];
                min_penalty = best_params.penalty;
            }
            result.params = current_params;
        }
        result.params = best_params;
        result.chosen.push_back(best_group_id);
    }
    return result.params.penalty;
}

void dfs(plan& current_plan, vector<plan>& contenders, size_t& result_penalty,
         const vector<vector<CONFLICT_TYPE>>& conflicts, const catalog& cat,
         const vector<vector<int>>& allowed, const prefs& preferences,
         int index, int end) {

    if (index == end) {
        size_t penalty = current_plan.params.penalty;

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
        plan_parameters current_parameters = current_plan.params;
        if (calculate_penalty(current_plan, group_id, preferences, conflicts,
                              cat) > result_penalty) {
            current_plan.params = current_parameters;
            continue;
        }

        current_plan.chosen.push_back(group_id);

        dfs(current_plan, contenders, result_penalty, conflicts, cat, allowed,
            preferences, index + 1, end);

        current_plan.chosen.pop_back();
        current_plan.params = current_parameters;
    }
}

vector<plan> generate_plans(const catalog& cat, prefs preferences,
                            const vector<vector<CONFLICT_TYPE>>& conflicts) {
    vector<plan> contenders;  // wszystkie równoważne najlepsze plany

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

    // Od razu usuwamy grupy niepasujące do preferencji
    for (vector<int> vec : allowed) {
        for (auto it = vec.begin(); it != vec.end(); ++it) {
            if (!group_fits_prefs(cat.groups[*it], preferences)) {
                vec.erase(it);
            }
        }
    }

    // size_t result_penalty = 4000000;  // każdy pełny plan ma być lepszy.
    size_t result_penalty =
        find_penalty_limit(allowed, cat, preferences, conflicts);

    cout << "RESULT_PENALTY: " << result_penalty << endl;

    // Sortujemy kursy w allowed rosnąco po liczbie grup, aby przyspieszyć odcinanie krawędzi w dfs.
    sort(allowed.begin(), allowed.end(),
         [&](vector<int> a, vector<int> b) { return a.size() < b.size(); });

    plan new_plan;
    dfs(new_plan, contenders, result_penalty, conflicts, cat, allowed,
        preferences, 0, cat.sub_courses.size());
    std::cout << "FINAL PENALTY: " << contenders[0].params.penalty << endl;

    return contenders;
}
