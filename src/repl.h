#pragma once

#include "model.h"
#include <tuple>
#include <vector>

// Wypisuje wszystkie grupy z ich id i wczytuje z cin id grup, które mają
// się znaleźć w planie (jeden na linię, pusta linia kończy).
std::vector<int> ask_for_groups(const catalog &cat);
