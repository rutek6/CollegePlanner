"""Konwertuje plan z USOSweb (.html) na format .in programu CollegePlanner.

Użycie: python3 usos_to_in.py plan.html > plan.in

Samo parsowanie robi parse_html_plan z parser.py; ten plik tylko wypisuje
jego wynik. Ostrzeżenia idą na stderr, dane .in na stdout.
"""

import sys

from parser import parse_html_plan

DAY_NAMES = ["Pn", "Wt", "Sr", "Cz", "Pt", "So", "Nd"]

# Typy z parsera -> typy znane programowi (enum type w model.h).
# Pozostałe (LEK_NOW, KON, WAR, UNK) trafiają do OTHER.
TYPE_MAP = {"CW": "CW", "CWW": "CWW", "WYK": "WYK", "LAB": "LAB", "WF": "WF"}


def warn(message):
    print("usos_to_in: " + message, file=sys.stderr)


def format_time(minutes):
    return f"{minutes // 60:02d}:{minutes % 60:02d}"


def valid_slots(course, group):
    """Terminy grupy bez powtórzeń, posortowane; błędne pomija z ostrzeżeniem."""
    result = set()
    for slot in group.slots:
        if slot.day is None or not 0 <= slot.day < 7:
            warn(f"{course.name} {group.key}: nieznany dzień - termin pominięty")
        elif slot.end is None or slot.end <= slot.start:
            warn(f"{course.name} {group.key}: niepoprawne godziny "
                 f"- termin pominięty")
        else:
            result.add((slot.day, slot.start, slot.end))  # ten sam termin
                                                          # z kilku tygodni
    return sorted(result)


def write_in(courses, out):
    for course in courses:
        # typ programu -> (typy z parsera, [(nr grupy, terminy)])
        by_type = {}
        for group in sorted(course.groups, key=lambda g: g.number):
            slots = valid_slots(course, group)
            if not slots:
                warn(f"{course.name} {group.key}: brak terminów - grupa "
                     f"pominięta")
                continue
            target = TYPE_MAP.get(group.type, "OTHER")
            sources, groups = by_type.setdefault(target, (set(), []))
            sources.add(group.type)
            groups.append((group.number, slots))
        if not by_type:
            warn(f"{course.name}: brak zajęć - przedmiot pominięty")
            continue

        # '#' zaczyna komentarz w formacie .in
        out.write(f"PRZEDMIOT {course.name.replace('#', '♯')}\n")
        for target, (sources, groups) in by_type.items():
            if len(sources) > 1:
                warn(f"{course.name}: typy {sorted(sources)} połączone w "
                     f"{target} - program potraktuje je jak jedne zajęcia")
            out.write(f"  ZAJECIA {target}\n")
            for number, slots in groups:
                out.write(f"    GRUPA {number}\n")
                for day, start, end in slots:
                    out.write(f"      {DAY_NAMES[day]} "
                              f"{format_time(start)}-{format_time(end)}\n")


def main():
    if len(sys.argv) != 2:
        print("Użycie: python3 usos_to_in.py plan.html > plan.in",
              file=sys.stderr)
        sys.exit(2)
    write_in(parse_html_plan(sys.argv[1]), sys.stdout)


if __name__ == "__main__":
    main()
