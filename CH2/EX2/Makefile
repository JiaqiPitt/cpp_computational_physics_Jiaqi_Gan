
CXX := clang++
CXXFLAGS := -std=c++20 -Wall -Wextra -pedantic
PROGRAMS := gamma

.PHONY: all clean

all: $(PROGRAMS)

gamma: gamma.cpp
	$(CXX) $(CXXFLAGS) $< -o $@

clean:
	rm -f $(PROGRAMS) *.o
