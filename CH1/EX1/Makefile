CXX := clang++
CXXFLAGS := -std=c++20 -Wall -Wextra -pedantic
PROGRAMS := exp exp_constant exp_constant_O2 exp_variable_O2 mangling

.PHONY: all clean

all: $(PROGRAMS)

exp: exp.cpp
	$(CXX) $(CXXFLAGS) $< -o $@

exp_constant: exp_constant.cpp
	$(CXX) $(CXXFLAGS) $< -o $@

exp_constant_O2: exp_constant.cpp
	$(CXX) $(CXXFLAGS) -O2 $< -o $@

exp_variable_O2: exp_variable.cpp
	$(CXX) $(CXXFLAGS) -O2 $< -o $@

mangling: mangling.cpp
	$(CXX) $(CXXFLAGS) -O0 $< -o $@

clean:
	rm -f $(PROGRAMS) *.o
