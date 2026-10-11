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

class group {
  public:
    std::vector<slot> slots;
    int nr;
    bool is_conflict(group second) const;
};

class sub_course {
  public:
    std::vector<group> groups;
    type class_type;
};

class course {
  public:
    std::vector<sub_course> sub_courses;
    std::string name;
};

class plan {
  public:
    // chosen[k] = indeks wybranej grupy w k-tym pod-przedmiocie
    // (kolejność: przedmioty -> pod-przedmioty, jak w dfs)
    std::vector<int> chosen;
    size_t conflicts = 0; // ustawiane przez solver
    size_t calculate_conflicts(const std::vector<sub_course> &sub_courses);
};