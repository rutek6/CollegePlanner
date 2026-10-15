#include "conflict.h"
#include "model.h"

#include <vector>

using namespace std;

vector<vector<CONFLICT_TYPE>> create_conflict_graph(const catalog &cat) {
    size_t n = cat.groups.size();
    vector<vector<CONFLICT_TYPE>> result(n, vector<CONFLICT_TYPE>(n, NONE));
    for (size_t i = 0; i < cat.groups.size(); i++) {
        for (size_t j = 0; j < cat.groups.size(); j++) {
            if (cat.groups[i].is_conflict(cat.groups[j])) {
                type group_i_type =
                    cat.sub_courses[cat.groups[i].sub_id].class_type;
                type group_j_type =
                    cat.sub_courses[cat.groups[j].sub_id].class_type;
                if (group_i_type == WYK || group_j_type == WYK) {
                    result[i][j] = SOFT;
                }
                else {
                    result[i][j] = HARD;
                }
            }
            else {
                result[i][j] = NONE;
            }
        }
    }
    return result;
}