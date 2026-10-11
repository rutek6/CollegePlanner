#pragma once

#include <vector>

#include "model.h"
#include "preferences.h"

// Zwraca plan z najmniejszą liczbą konfliktów. preferences.required_group_ids
// to numery grup w kolejności: przedmioty -> pod-przedmioty -> grupy (jak w
// ask_for_groups). W pod-przedmiocie, w którym wybrano jakąś grupę,
// rozważane są tylko wybrane grupy.
plan generate_plans(std::vector<course> selected_courses, prefs preferences);
