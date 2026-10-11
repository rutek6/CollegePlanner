#pragma once

#include <cstddef>
#include <string>
#include <vector>

enum type { CW, LAB, WYK, CWW, WF, SEM, OTHER };

// Nazwy typów zajęć i dni tygodnia - indeksowane wartościami
// enum type oraz slot::day. Używane przy wczytywaniu i wypisywaniu.
extern const char *type_names[];
extern const int type_count;
extern const char *day_names[];
extern const int day_count;

class slot {
  public:
    int start; // minutes from 00:00
    int end;
    int day; // Monday - 0, Friday - 4;

    bool is_conflict(slot second) const;
};

// Numeracja: przedmioty, pod-przedmioty i grupy leżą w osobnych, płaskich
// wektorach w catalog. Id obiektu = jego indeks w tym wektorze - to jedyny
// system numeracji w programie (używają go repl, solver i output).
// Obiekty wskazują na siebie nawzajem przez id (w dół i w górę).

class group {
  public:
    std::vector<slot> slots;
    int nr;     // numer grupy z USOS - tylko etykieta do wypisania, NIE id
    int sub_id; // pod-przedmiot, do którego należy grupa
    bool is_conflict(group second) const;
};

class sub_course {
  public:
    std::vector<int> group_ids;
    type class_type;
    int course_id; // przedmiot, do którego należy pod-przedmiot
};

class course {
  public:
    std::vector<int> sub_ids;
    std::string name;
};

class catalog {
  public:
    std::vector<course> courses;
    std::vector<sub_course> sub_courses;
    std::vector<group> groups;
};

class plan {
  public:
    // chosen[k] = id grupy wybranej w pod-przedmiocie o id k
    std::vector<int> chosen;
    size_t conflicts = 0; // ustawiane przez solver
    size_t calculate_conflicts(const std::vector<std::vector<bool>> &conflicts);
};