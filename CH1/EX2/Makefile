CXX := clang++
CXXFLAGS := -std=c++20 -Wall -Wextra -pedantic
PROGRAM := exp

.PHONY: all clean

all: $(PROGRAM)

$(PROGRAM): exp.cpp
	$(CXX) $(CXXFLAGS) $< -o $@

clean:
	rm -f $(PROGRAM) *.o
