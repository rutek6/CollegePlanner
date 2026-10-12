#pragma once

#include "model.h"

// Interfejs w terminalu (FTXUI) na pełnym ekranie: plan p jako siatka
// tygodnia (dni x godziny), konflikty na czerwono. Wraca, gdy użytkownik
// zakończy program (klawisz 'q' albo Esc).
void run_tui(const catalog &cat, const plan &p);
