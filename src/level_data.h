#ifndef LEVEL_DATA_H
#define LEVEL_DATA_H

#include "math.h"
#include "player.h"
#include <string>
#include <vector>

struct LevelData {
  std::string name;
  Vec3 spawn;
  std::string next_level;
  Vec3 star_pos;
  std::vector<PlatformData> platforms;
  std::vector<Vec3> coins;
};

#endif // LEVEL_DATA_H
