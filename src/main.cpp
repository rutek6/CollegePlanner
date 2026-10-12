#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "conflict.h"
#include "input_text.h"
#include "input_usos.h"
#include "model.h"
#include "output.h"
#include "preferences.h"
#include "solver.h"
#include "tui.h"

using namespace std;

int main(int argc, char *argv[]) {
    if (argc < 2) {
        cerr << "Użycie " << argv[0]
             << " <plik z danymi .in albo plan z USOS .html> [preferencje]\n";
        return 1;
    }
    string data_path = argv[1];
    // Panel preferencji w TUI zapisuje do tego pliku.
    string prefs_path = argc >= 3 ? argv[2] : "prefs.in";

    catalog cat;
    prefs preferences;
    try {
        string warnings;
        cat = load_catalog(data_path, warnings);
        cerr << warnings;
        if (argc == 3) {
            ifstream input(argv[2]);
            if (!input) {
                cerr << "Nie można otworzyć pliku \"" << argv[2] << "\"\n";
                return 1;
            }
            read_prefs(preferences, cat, input, cout);
        }
    } catch (const input_exception &e) {
        cerr << e.what() << '\n';
        return 1;
    }

    vector<vector<bool>> conflicts = create_conflict_graph(cat);
    vector<plan> best = generate_plans(cat, preferences, conflicts);
    // print_plan(best, cat, cout);
    run_tui(data_path, cat, conflicts, preferences, best, prefs_path);
}
