CXX := clang++
CXXFLAGS := -std=c++20 -Wall -Wextra -pedantic
PROGRAM := foo

.PHONY: all clean

all: $(PROGRAM)

$(PROGRAM): foo.cpp
	$(CXX) $(CXXFLAGS) $< -o $@

clean:
	rm -f $(PROGRAM) *.o
