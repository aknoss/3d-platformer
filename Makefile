CXX = g++
CXXFLAGS = -Wall -Wextra -O2 $(shell pkg-config --cflags sdl3)
LDFLAGS = $(shell pkg-config --libs sdl3) -lGL -lm

SRCS = src/main.cpp src/renderer.cpp src/player.cpp src/level.cpp src/hud.cpp

game: $(SRCS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

clean:
	rm -f game

.PHONY: clean
