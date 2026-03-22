CXX = g++
CXXFLAGS = -Wall -Wextra -O2 -MMD -MP $(shell pkg-config --cflags sdl3)
LDFLAGS = $(shell pkg-config --libs sdl3) -lGL -lm

SRCS = src/main.cpp src/renderer.cpp src/player.cpp src/level.cpp src/hud.cpp
OBJS = $(patsubst src/%.cpp,build/%.o,$(SRCS))
GENERATED = build/character_obj.h build/character_tex.h

build/game: $(OBJS)
	$(CXX) -o $@ $^ $(LDFLAGS)

build/%.o: src/%.cpp $(GENERATED) | build
	$(CXX) $(CXXFLAGS) -Ibuild -c -o $@ $<

build/character_obj.h: assets/character.obj | build
	xxd -i $< > $@

build/character_tex.h: assets/texture-character.png | build
	xxd -i $< > $@

build:
	mkdir -p build

clean:
	rm -rf build

-include $(OBJS:.o=.d)

.PHONY: clean
