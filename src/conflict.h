#pragma once

#include <istream>
#include <string>
#include <vector>

#include "model.h"

std::vector<std::vector<bool>> create_conflict_graph(const catalog &cat);