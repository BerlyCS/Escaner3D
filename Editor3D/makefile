CXX      := g++
CXXFLAGS := -Wall -Wextra -std=c++17 -O2
LDFLAGS  := -lGL -lglut -lGLU

OBJS     := main.o camera.o model.o texture.o

editor_vis: $(OBJS)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

run: editor_vis
	./editor_vis

.PHONY: clean run

clean:
	rm -f editor_vis *.o
