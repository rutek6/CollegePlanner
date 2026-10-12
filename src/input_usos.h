#pragma once

#include <string>

#include "model.h"

// Wczytuje dane z pliku. Plik .html/.htm (plan z USOSweb) przechodzi przez
// parser w Pythonie (src/usos_to_in.py, obok pliku wykonywalnego), który
// zamienia go na format .in; pozostałe pliki są czytane jako .in.
// Interpreter: python3 albo zmienna środowiskowa PLANNER_PYTHON (np. python
// z virtualenva z bs4). Ostrzeżenia parsera (pominięte terminy itp.) trafiają
// do warnings. Przy błędzie rzuca input_exception z komunikatem.
catalog load_catalog(const std::string &path, std::string &warnings);
