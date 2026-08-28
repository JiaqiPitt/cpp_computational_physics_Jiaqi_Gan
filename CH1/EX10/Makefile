CXX := clang++
CXXFLAGS := -std=c++20 -Wall -Wextra -pedantic
PROGRAMS := legendre hermite

.PHONY: all clean

all: $(PROGRAMS)

legendre: legendre.cpp
	$(CXX) $(CXXFLAGS) $< -o $@

hermite: hermite.cpp
	$(CXX) $(CXXFLAGS) $< -o $@

clean:
	rm -f $(PROGRAMS) *.o
