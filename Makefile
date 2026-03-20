CXX = g++
CXXFLAGS = -Wall -Wextra -O2 $(shell pkg-config --cflags sdl3)
LDFLAGS = $(shell pkg-config --libs sdl3) -lGL -lm

game: main.cpp
	$(CXX) $(CXXFLAGS) -o $@ $< $(LDFLAGS)

clean:
	rm -f platformer

.PHONY: clean
