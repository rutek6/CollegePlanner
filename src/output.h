#pragma once

#include <ostream>
#include <string>
#include <vector>

#include "model.h"

// minuty od 00:00 -> "HH:MM"
std::string format_time(int minutes);

// Wypisuje terminy wybranych grup (p.chosen - id grup) posortowane według
// dnia i godziny rozpoczęcia.
void print_plan(const plan &p, const catalog &cat, std::ostream &out);
