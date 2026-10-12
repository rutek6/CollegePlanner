#pragma once

#include <string>
#include <vector>

#include "model.h"
#include "preferences.h"

// Interfejs w terminalu (FTXUI) na pełnym ekranie: plan jako siatka tygodnia
// (dni x godziny), konflikty na czerwono. Przy kilku równoważnych planach
// strzałki (←/→, Home/End) przełączają między nimi.
// - 'p' otwiera panel preferencji: po zapisie preferencje trafiają do pliku
//   prefs_path (i do preferences), a plany są liczone od nowa,
// - 'o' otwiera inny plik z danymi (.html z USOSweb albo .in).
// Dane (cat, conflicts, plans) interfejs dostaje na własność, bo po
// wczytaniu nowego pliku je podmienia. Wraca, gdy użytkownik zakończy
// program (klawisz 'q' albo Esc).
void run_tui(std::string data_path,
             catalog cat,
             std::vector<std::vector<bool>> conflicts,
             prefs &preferences,
             std::vector<plan> plans,
             const std::string &prefs_path);
