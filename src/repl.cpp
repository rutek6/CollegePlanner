#include "repl.h"

#include <iostream>
#include <stdexcept>
#include <string>
#include <tuple>

#include "input_text.h"

using namespace std;

vector<int> ask_for_groups(const catalog &cat) {
    cout << "\nWybierz grupy: ";
    for (const course &c : cat.courses) {
        cout << "\n" << c.name;
        for (int sub_id : c.sub_ids) {
            const sub_course &sub = cat.sub_courses[sub_id];
            for (int group_id : sub.group_ids) {
                cout << "\n"
                     << group_id << ": " << cat.groups[group_id].nr << " - "
                     << type_names[sub.class_type];
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
            if (parsed != line.size() || id < 0 ||
                id >= (int)cat.groups.size()) {
                throw out_of_range(line);
            }
            result.push_back(id);
        } catch (const exception &) {
            cerr << "Niepoprawny numer grupy: \"" << line << "\"\n";
        }
    }
    return result;
}
