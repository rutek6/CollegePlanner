#pragma once

#include <istream>
#include <string>
#include <vector>

#include "model.h"

std::vector<std::vector<CONFLICT_TYPE>>
create_conflict_graph(const catalog &cat);