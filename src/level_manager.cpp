#include "level_manager.h"
#include <algorithm>
#include <dirent.h>
#include <fstream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

LevelManager::LevelManager() {
  DIR *dir = opendir("assets/levels");
  if (dir) {
    struct dirent *entry;
    while ((entry = readdir(dir)) != nullptr) {
      std::string name = entry->d_name;
      if (name.size() > 5 && name.substr(name.size() - 5) == ".json") {
        std::string key = name.substr(0, name.size() - 5);
        std::string path = "assets/levels/" + name;
        levels_[key] = parse_json(path);
        level_keys_.push_back(key);
      }
    }
    closedir(dir);
  }

  std::sort(level_keys_.begin(), level_keys_.end());

  if (level_keys_.empty()) {
    levels_["default"] = default_level();
    level_keys_.push_back("default");
  }

  load_level(level_keys_.front());
}

bool LevelManager::load_level(const std::string &key) {
  auto it = levels_.find(key);
  if (it == levels_.end())
    return false;
  current_data_ = it->second;
  current_key_ = key;
  level_ = Level(current_data_);
  return true;
}

bool LevelManager::check_star_collected() {
  if (!level_.star_collected())
    return false;
  if (current_data_.next_level.empty())
    return false;
  return load_level(current_data_.next_level);
}

LevelData LevelManager::parse_json(const std::string &filepath) {
  std::ifstream f(filepath);
  json j = json::parse(f);

  LevelData data;
  data.name = j.value("name", "");

  auto spawn = j.at("spawn");
  data.spawn = {spawn[0].get<float>(), spawn[1].get<float>(),
                spawn[2].get<float>()};

  data.next_level = j.value("next_level", "");

  auto star = j.at("star");
  data.star_pos = {star[0].get<float>(), star[1].get<float>(),
                   star[2].get<float>()};

  for (const auto &p : j.at("platforms")) {
    PlatformData pd;
    auto mn = p.at("min");
    auto mx = p.at("max");
    auto col = p.at("color");
    pd.min = {mn[0].get<float>(), mn[1].get<float>(), mn[2].get<float>()};
    pd.max = {mx[0].get<float>(), mx[1].get<float>(), mx[2].get<float>()};
    pd.color = {col[0].get<float>(), col[1].get<float>(), col[2].get<float>()};
    pd.pushable = p.value("pushable", false);
    data.platforms.push_back(pd);
  }

  if (j.contains("coins")) {
    for (const auto &c : j.at("coins")) {
      data.coins.push_back(
          {c[0].get<float>(), c[1].get<float>(), c[2].get<float>()});
    }
  }

  return data;
}

LevelData LevelManager::default_level() {
  LevelData data;
  data.name = "Default";
  data.spawn = {0, 1, 0};
  data.star_pos = {-8, 5.5f, -8.5f};

  data.platforms = {
      {{-15, -0.5f, -15}, {15, 0, 15}, {0.2f, 0.7f, 0.2f}, false},
      {{3, 0, 3}, {7, 1.5f, 7}, {0.6f, 0.4f, 0.2f}, true},
      {{-8, 0, -3}, {-4, 2.5f, 1}, {0.5f, 0.5f, 0.5f}, true},
      {{-3, 0, 8}, {1, 1.0f, 12}, {0.6f, 0.4f, 0.2f}, true},
      {{8, 0, -8}, {12, 3.5f, -4}, {0.5f, 0.5f, 0.5f}, true},
      {{-10, 0, -10}, {-6, 4.0f, -7}, {0.6f, 0.4f, 0.2f}, true},
      {{5, 0, -2}, {9, 2.0f, 2}, {0.5f, 0.5f, 0.5f}, true},
  };

  data.coins = {
      {5, 2.5f, 5},      {-6, 3.5f, -1}, {-1, 2.0f, 10}, {10, 4.5f, -6},
      {-8, 5.0f, -8.5f}, {7, 3.0f, 0},   {0, 1.0f, 0},   {-12, 1.0f, 12},
  };

  return data;
}
