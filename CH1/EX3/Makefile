
CXX := clang++
CXXFLAGS := -std=c++20 -Wall -Wextra -pedantic
PROGRAMS := custom_exp system_exp

.PHONY: all clean

all: $(PROGRAMS)

custom_exp: custom_exp.cpp
	$(CXX) $(CXXFLAGS) $< -o $@

system_exp: system_exp.cpp
	$(CXX) $(CXXFLAGS) $< -o $@

clean:
	rm -f $(PROGRAMS) *.o
