#pragma once

#include <tuple>
#include <vector>

using namespace std;

struct prefs {
    tuple<int, int> hours[5] = {{0, 24 * 60}, {0, 24 * 60}, {0, 24 * 60}, {0, 24 * 60}, {0, 24 * 60}};
    bool free_days[5] = {false, false, false, false, false};
    vector<int> required_group_ids;
};