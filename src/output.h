#pragma once

#include <ostream>
#include <string>
#include <vector>

#include "model.h"

// minuty od 00:00 -> "HH:MM"
std::string format_time(int minutes);

// p.chosen[k] to indeks grupy w k-tym pod-przedmiocie, w kolejności
// w jakiej pod-przedmioty występują na liście przedmiotów (jak w dfs).
void print_plan(const plan &p,
                const std::vector<course> &courses,
                std::ostream &out);
