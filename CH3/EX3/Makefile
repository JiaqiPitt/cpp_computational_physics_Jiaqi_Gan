
CXX := clang++
CXXFLAGS := -std=c++20 -Wall -Wextra -pedantic
PROGRAMS := multi_barrier

.PHONY: all clean

all: $(PROGRAMS)

multi_barrier: multi_barrier.cpp
	$(CXX) $(CXXFLAGS) $< -o $@

clean:
	rm -f $(PROGRAMS) *.o
