CXX = g++
CXXFLAGS = -std=c++17 -O2 -Wall -Wextra -MMD -MP

OBJ = test.o
LDLIBS = -ljsoncpp -lcurl -lcpr

test: $(OBJ)
	$(CXX) $(CXXFLAGS) -c $< -o $@
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDLIBS)

	