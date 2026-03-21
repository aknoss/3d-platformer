CXX = g++
CXXFLAGS = -Wall -Wextra -O2 -MMD -MP $(shell pkg-config --cflags sdl3)
LDFLAGS = $(shell pkg-config --libs sdl3) -lGL -lm

SRCS = src/main.cpp src/renderer.cpp src/player.cpp src/level.cpp src/hud.cpp
OBJS = $(patsubst src/%.cpp,build/%.o,$(SRCS))

build/game: $(OBJS)
	$(CXX) -o $@ $^ $(LDFLAGS)

build/%.o: src/%.cpp | build
	$(CXX) $(CXXFLAGS) -c -o $@ $<

build:
	mkdir -p build

clean:
	rm -rf build

-include $(OBJS:.o=.d)

.PHONY: clean
