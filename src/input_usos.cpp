#include "input_usos.h"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <unistd.h>

#include "input_text.h"

using namespace std;

bool is_html(const string &path) {
    string lower = path;
    transform(lower.begin(), lower.end(), lower.begin(),
              [](unsigned char c) { return tolower(c); });
    for (string suffix : {".html", ".htm"}) {
        if (lower.size() >= suffix.size() &&
            lower.compare(lower.size() - suffix.size(), suffix.size(),
                          suffix) == 0) {
            return true;
        }
    }
    return false;
}

// src/usos_to_in.py względem pliku wykonywalnego (./solver leży w katalogu
// projektu), żeby działało niezależnie od katalogu, z którego go uruchomiono.
string converter_path() {
    char buffer[4096];
    ssize_t length = readlink("/proc/self/exe", buffer, sizeof(buffer) - 1);
    string dir = ".";
    if (length > 0) {
        buffer[length] = '\0';
        dir = buffer;
        dir = dir.substr(0, dir.rfind('/'));
    }
    return dir + "/src/usos_to_in.py";
}

// Argument dla powłoki w apostrofach (apostrof w środku -> '\'').
string shell_quote(const string &s) {
    string result = "'";
    for (char c : s) {
        if (c == '\'') {
            result += "'\\''";
        }
        else {
            result += c;
        }
    }
    return result + "'";
}

// Uruchamia parser w Pythonie; zwraca dane w formacie .in. Stderr parsera
// zbieramy w pliku tymczasowym: przy błędzie jego ostatnia linia (np.
// "ModuleNotFoundError: No module named 'bs4'") trafia do komunikatu,
// a przy sukcesie to ostrzeżenia.
string convert_usos_html(const string &path, string &warnings) {
    char error_path[] = "/tmp/collegeplanner-XXXXXX";
    int error_fd = mkstemp(error_path);
    if (error_fd < 0) {
        throw input_exception("Nie można utworzyć pliku tymczasowego");
    }
    close(error_fd);

    const char *python = getenv("PLANNER_PYTHON");
    string command = shell_quote(python ? python : "python3") + " " +
                     shell_quote(converter_path()) + " " + shell_quote(path) +
                     " 2>" + shell_quote(error_path);
    FILE *pipe = popen(command.c_str(), "r");
    if (!pipe) {
        remove(error_path);
        throw input_exception("Nie można uruchomić parsera USOS");
    }
    string output;
    char chunk[4096];
    size_t read;
    while ((read = fread(chunk, 1, sizeof(chunk), pipe)) > 0) {
        output.append(chunk, read);
    }
    int status = pclose(pipe);

    ifstream error_file(error_path);
    stringstream errors;
    errors << error_file.rdbuf();
    remove(error_path);

    if (status != 0) {
        string last_line;
        string line;
        while (getline(errors, line)) {
            if (!trim(line).empty()) {
                last_line = trim(line);
            }
        }
        throw input_exception("Parser USOS (" + converter_path() +
                              ") zakończył się błędem: " + last_line);
    }
    warnings = errors.str();
    return output;
}

catalog load_catalog(const string &path, string &warnings) {
    warnings.clear();
    ifstream file(path);
    if (!file) {
        throw input_exception("Nie można otworzyć pliku \"" + path + "\"");
    }
    if (!is_html(path)) {
        return read_catalog(file);
    }
    istringstream converted(convert_usos_html(path, warnings));
    catalog cat = read_catalog(converted);
    if (cat.courses.empty()) {
        throw input_exception("W pliku \"" + path +
                              "\" nie znaleziono żadnych zajęć");
    }
    return cat;
}
