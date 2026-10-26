CXX      = g++
CXXFLAGS = -std=c++17 -O2 -Wall -Wextra
CPPFLAGS = -MMD -MP
# Wszystkie .cpp z src/ i src/tui/. Obiekty lądują w build/ z zachowaniem
# podkatalogów (src/tui/tui.cpp -> build/tui/tui.o), więc nazwy się nie gryzą.
SRC      = $(wildcard src/*.cpp)
OBJ      = $(SRC:src/%.cpp=build/%.o)
# FTXUI (AUR: yay -S ftxui). Kolejność ma znaczenie: component -> dom -> screen
LDLIBS   = -lftxui-component -lftxui-dom -lftxui-screen

solver: $(OBJ)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDLIBS)

build/%.o: src/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c $< -o $@

clean:
	rm -rf build solver

.PHONY: clean

-include $(OBJ:.o=.d)
