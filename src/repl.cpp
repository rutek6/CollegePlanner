#include "repl.h"

#include <iostream>
#include <stdexcept>
#include <string>
#include <tuple>

#include "input_text.h"

using namespace std;

vector<int> ask_for_groups(const vector<course> &courses) {
    int index = 0;
    cout << "\nWybierz grupy: ";
    for (course c : courses) {
        cout << "\n" << c.name;
        for (sub_course sub : c.sub_courses) {
            for (group g : sub.groups) {
                cout << "\n"
                     << index << ": " << g.nr << " - "
                     << type_names[sub.class_type];
                index++;
            }
        }
    }
    cout << endl;
    string line;
    vector<int> result;
    while (getline(cin, line)) {
        line = trim(line);
        if (line.empty()) {
            break;
        }
        try {
            size_t parsed;
            int id = stoi(line, &parsed);
            if (parsed != line.size() || id < 0 || id >= index) {
                throw out_of_range(line);
            }
            result.push_back(id);
        } catch (const exception &) {
            cerr << "Niepoprawny numer grupy: \"" << line << "\"\n";
        }
    }
    return result;
}
