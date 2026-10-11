CXX      = g++
CXXFLAGS = -std=c++17 -O2 -Wall -Wextra -MMD -MP
SRC      = $(wildcard src/*.cpp)
OBJ      = $(SRC:src/%.cpp=build/%.o)

solver: $(OBJ)
	$(CXX) $(CXXFLAGS) $^ -o $@

build/%.o: src/%.cpp | build
	$(CXX) $(CXXFLAGS) -c $< -o $@

build:
	mkdir -p build

clean:
	rm -rf build solver

.PHONY: clean

-include $(OBJ:.o=.d)
