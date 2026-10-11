#include <fstream>
#include <iostream>
#include <vector>

#include "input_text.h"
#include "model.h"
#include "output.h"
#include "preferences.h"
#include "repl.h"
#include "solver.h"

using namespace std;

int main(int argc, char *argv[]) {
    if (argc < 2) {
        cerr << "Użycie " << argv[0] << " <plik z danymi>\n";
        return 1;
    }
    ifstream input(argv[1]);
    if (!input) {
        cerr << "Nie można otworzyć pliku \"" << argv[1] << "\"\n";
        return 1;
    }
    vector<course> selected_courses = read_courses(input);
    // vector<int> required_group_ids = ask_for_groups(selected_courses);
    prefs preferences;
    preferences.required_group_ids = ask_for_groups(selected_courses);

    if (argc == 3) {
        ifstream input(argv[2]);
        if (!input) {
            cerr << "Nie można otworzyć pliku \"" << argv[1] << "\"\n";
            return 1;
        }
        read_prefs(preferences.hours, preferences.free_days, input);
    }

    plan best = generate_plans(selected_courses, preferences);
    print_plan(best, selected_courses, cout);
}
