CXX      = g++
CXXFLAGS = -std=c++17 -O2 -Wall -Wextra -MMD -MP -Wunused
SRC      = $(wildcard src/*.cpp)
OBJ      = $(SRC:src/%.cpp=build/%.o)
# FTXUI (AUR: yay -S ftxui). Kolejność ma znaczenie: component -> dom -> screen
LDLIBS   = -lftxui-component -lftxui-dom -lftxui-screen

solver: $(OBJ)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDLIBS)

build/%.o: src/%.cpp | build
	$(CXX) $(CXXFLAGS) -c $< -o $@

build:
	mkdir -p build

clean:
	rm -rf build solver

.PHONY: clean

-include $(OBJ:.o=.d)
