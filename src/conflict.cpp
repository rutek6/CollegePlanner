#include "conflict.h"
#include "model.h"

#include <vector>

using namespace std;

vector<vector<bool>> create_conflict_graph(const catalog &cat) {
    vector<vector<bool>> result;
    for (size_t i = 0; i < cat.groups.size(); i++) {
        for (size_t j = 0; j < cat.groups.size(); j++) {
            if (cat.groups[i].is_conflict(cat.groups[j])) {
                result[i][j] = true;
            }
            else {
                result[i][j] = false;
            }
        }
    }
    return result;
}