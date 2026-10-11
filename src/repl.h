#pragma once

#include "model.h"
#include <tuple>
#include <vector>

// Wypisuje wszystkie grupy z numerami i wczytuje z cin numery grup,
// które mają się znaleźć w planie (jeden na linię, pusta linia kończy).
std::vector<int> ask_for_groups(const std::vector<course> &courses);
