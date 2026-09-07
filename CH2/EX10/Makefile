
CXX := clang++
CXXFLAGS := -std=c++20 -Wall -Wextra -pedantic -isystem /opt/homebrew/opt/eigen@3/include/eigen3
PROGRAMS := diagonalize

.PHONY: all clean

all: $(PROGRAMS)

diagonalize: diagonalize.cpp
	$(CXX) $(CXXFLAGS) $< -o $@

clean:
	rm -f $(PROGRAMS) *.o
