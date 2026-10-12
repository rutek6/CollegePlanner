#pragma once

#include <istream>
#include <stdexcept>
#include <string>
#include <vector>

#include "model.h"
#include "preferences.h"

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
// Obiekty dostają id w kolejności występowania w pliku, więc grupy jednego
// pod-przedmiotu (i pod-przedmioty jednego przedmiotu) mają kolejne id.
//
// Przy błędnych danych rzuca input_exception z komunikatem (z numerem linii).
// ---------------------------------------------------------------------------
// Błąd w danych wejściowych (format .in albo preferencji); what() to gotowy
// komunikat dla użytkownika.
struct input_exception : std::runtime_error {
    using std::runtime_error::runtime_error;
};

catalog read_catalog(std::istream &in);

// Usuwa białe znaki z początku i końca napisu.
std::string trim(const std::string &s);

void read_prefs(prefs &preferences,
                const catalog &cat,
                std::istream &in,
                std::ostream &out);

// Zapisuje preferencje w formacie czytanym przez read_prefs (godziny, wolne
// dni, przypięte grupy). Przypięcie zapisuje pełną nazwę przedmiotu bez
// spacji, np. "Przypnij AnalizaIII CW 7".
void write_prefs(const prefs &preferences,
                 const catalog &cat,
                 std::ostream &out);

// "HH:MM-HH:MM" (dopuszczalne spacje wokół '-') -> minuty od 00:00;
// false, gdy napis nie ma takiej postaci.
bool parse_time_range(const std::string &s, int &start, int &end);
