CXX := clang++
CXXFLAGS := -std=c++20 -Wall -Wextra -pedantic

AR := ar
ARFLAGS := rcs

PROGRAM := plus
LIBRARY := libPlus.a
LIB_OBJECTS := parse.o add.o print.o

.PHONY: all clean

all: $(LIBRARY) $(PROGRAM)

$(LIBRARY): $(LIB_OBJECTS)
	$(AR) $(ARFLAGS) $@ $^

$(PROGRAM): main.o $(LIBRARY)
	$(CXX) $(CXXFLAGS) main.o -L. -lPlus -o $@

%.o: %.cpp plus.hpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f *.o $(LIBRARY) $(PROGRAM)
