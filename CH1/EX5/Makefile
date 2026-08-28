CXX := clang++
CXXFLAGS := -std=c++20 -Wall -Wextra -pedantic
PROGRAM := plus

.PHONY: all clean

all: $(PROGRAM)

$(PROGRAM): plus.cpp
	$(CXX) $(CXXFLAGS) $< -o $@

clean:
	rm -f $(PROGRAM) *.o
