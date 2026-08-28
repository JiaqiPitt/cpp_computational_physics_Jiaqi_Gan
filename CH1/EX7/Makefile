CXX := clang++
CXXFLAGS := -std=c++20 -Wall -Wextra -pedantic

PROGRAM := plus
LIBRARY := libPlus.dylib
LIB_OBJECTS := parse.o add.o print.o

.PHONY: all clean

all: $(LIBRARY) $(PROGRAM)

$(LIBRARY): $(LIB_OBJECTS)
	$(CXX) -dynamiclib $^ -o $@ -Wl,-install_name,@rpath/$@

$(PROGRAM): main.o $(LIBRARY)
	$(CXX) $(CXXFLAGS) main.o -L. -lPlus \
		-Wl,-rpath,@executable_path -o $@

%.o: %.cpp plus.hpp
	$(CXX) $(CXXFLAGS) -fPIC -c $< -o $@

clean:
	rm -f *.o $(LIBRARY) $(PROGRAM)
