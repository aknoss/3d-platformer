#ifndef LEVEL_MANAGER_H
#define LEVEL_MANAGER_H

#include "level.h"
#include "level_data.h"
#include <string>
#include <unordered_map>
#include <vector>

class LevelManager {
public:
  LevelManager();

  bool load_level(const std::string &key);
  bool check_star_collected();

  Level &current_level() { return level_; }
  const Level &current_level() const { return level_; }
  Vec3 spawn_pos() const { return current_data_.spawn; }
  const std::vector<std::string> &level_keys() const { return level_keys_; }
  const std::string &current_key() const { return current_key_; }

private:
  std::unordered_map<std::string, LevelData> levels_;
  std::vector<std::string> level_keys_;
  LevelData current_data_;
  Level level_;
  std::string current_key_;

  LevelData parse_json(const std::string &filepath);
  static LevelData default_level();
};

#endif // LEVEL_MANAGER_H
