#pragma once

#include <vector>

#include "model.h"
#include "preferences.h"

// Zwraca plan z najmniejszą liczbą konfliktów. preferences.required_group_ids
// to id grup (indeksy w cat.groups). W pod-przedmiocie, w którym wybrano
// jakąś grupę, rozważane są tylko wybrane grupy.
plan generate_plans(const catalog &cat, prefs preferences);
