
CXX := clang++
CXXFLAGS := -std=c++20 -Wall -Wextra -pedantic
PROGRAMS := barrier

.PHONY: all clean

all: $(PROGRAMS)

barrier: barrier.cpp
	$(CXX) $(CXXFLAGS) $< -o $@

clean:
	rm -f $(PROGRAMS) *.o
