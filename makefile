CXX      := g++
CXXFLAGS := -Wall -Wextra -std=c++17 -O2
LDFLAGS  := -lGL -lglut -lGLU

OBJS     := main.o visor.o

main: $(OBJS)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

run: main
	./main

.PHONY: clean run

clean:
	rm -f main *.o
