#pragma once

#include <istream>
#include <string>
#include <vector>

#include "model.h"

// ---------------------------------------------------------------------------
// Wczytywanie danych
//
// Format wejścia - jedna informacja na linię, wcięcia są dowolne,
// puste linie oraz wszystko od znaku '#' do końca linii jest ignorowane:
//
//   PRZEDMIOT <nazwa, może zawierać spacje>
//   ZAJECIA <typ: WYK | CW | LAB | CWW | WF | SEM | OTHER>
//   GRUPA <nr>
//   <dzień: Pn | Wt | Sr | Cz | Pt | So | Nd> <HH:MM>-<HH:MM>
//
// ZAJECIA należą do ostatniego PRZEDMIOTU, GRUPA do ostatnich ZAJEC,
// a linia z dniem tygodnia to termin (slot) ostatniej GRUPY.
//
// Przy błędnych danych wypisuje komunikat na stderr i kończy program.
// ---------------------------------------------------------------------------
std::vector<course> read_courses(std::istream &in);

// Usuwa białe znaki z początku i końca napisu.
std::string trim(const std::string &s);

void read_prefs(std::tuple<int, int> *hours, bool *free_days, std::istream &in);