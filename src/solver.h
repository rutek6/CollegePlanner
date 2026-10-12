#pragma once

#include <vector>

#include "model.h"
#include "preferences.h"

// Zwraca wszystkie równoważne najlepsze plany: najpierw najmniej konfliktów,
// przy remisie najniższy time_cost; pusty wektor, gdy żaden plan nie spełnia
// preferencji. Każdy plan ma ustawione pole conflicts.
// preferences.required_group_ids
// to id grup (indeksy w cat.groups). W pod-przedmiocie, w którym wybrano
// jakąś grupę, rozważane są tylko wybrane grupy.
vector<plan> generate_plans(const catalog &cat,
                            prefs preferences,
                            const vector<vector<bool>> &conflicts);
