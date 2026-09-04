
CXX := clang++
CXXFLAGS := -std=c++20 -Wall -Wextra -pedantic
PROGRAMS := identities

.PHONY: all clean

all: $(PROGRAMS)

identities: identities.cpp
	$(CXX) $(CXXFLAGS) $< -o $@

clean:
	rm -f $(PROGRAMS) *.o
