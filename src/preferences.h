#pragma once

#include <tuple>
#include <unordered_map>
#include <vector>

enum weights {
    HARD_CONFLICT,
    SOFT_CONFLICT,
    TIME,
    TIME_SPENT,
    EARLY_START,
    LATE_END
};

struct prefs {
    std::tuple<int, int> hours[5] = {{0, 24 * 60},
                                     {0, 24 * 60},
                                     {0, 24 * 60},
                                     {0, 24 * 60},
                                     {0, 24 * 60}};
    bool free_days[5] = {false, false, false, false, false};
    std::vector<int>
        required_group_ids;  // id grup, czyli indeksy w catalog::groups
    std::unordered_map<weights, size_t> weights_map = {
        {HARD_CONFLICT, 1000000}, {SOFT_CONFLICT, 100000}, {TIME, 10},
        {TIME_SPENT, 5},          {EARLY_START, 0},        {LATE_END, 5}};
};