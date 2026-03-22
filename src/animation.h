#ifndef ANIMATION_H
#define ANIMATION_H

#include "math.h"
#include <string>
#include <vector>

struct NodeChannels {
  std::string node_name;
  std::vector<float> trans_times;
  std::vector<Vec3> trans_values;
  std::vector<float> rot_times;
  std::vector<Quat> rot_values;
  std::vector<float> scale_times;
  std::vector<Vec3> scale_values;
};

struct AnimClip {
  std::string name;
  float duration;
  std::vector<NodeChannels> channels;

  const NodeChannels *find_node(const std::string &name) const {
    for (const auto &ch : channels) {
      if (ch.node_name == name)
        return &ch;
    }
    return nullptr;
  }
};

struct AnimSet {
  std::vector<AnimClip> clips;

  const AnimClip *find_clip(const std::string &name) const {
    for (const auto &c : clips) {
      if (c.name == name)
        return &c;
    }
    return nullptr;
  }
};

struct NodePose {
  Vec3 translation;
  Quat rotation;
  Vec3 scale;
  NodePose() : scale(1, 1, 1) {}
};

// Find index i such that times[i] <= t < times[i+1]
inline int find_keyframe_index(const std::vector<float> &times, float t) {
  if (times.size() < 2)
    return 0;
  if (t <= times[0])
    return 0;
  if (t >= times[times.size() - 1])
    return (int)times.size() - 2;
  // Linear search (clips are typically short)
  for (int i = 0; i < (int)times.size() - 1; i++) {
    if (t < times[i + 1])
      return i;
  }
  return (int)times.size() - 2;
}

inline Vec3 sample_vec3_channel(const std::vector<float> &times,
                                const std::vector<Vec3> &values, float t) {
  if (values.empty())
    return {0, 0, 0};
  if (values.size() == 1 || t <= times[0])
    return values[0];
  if (t >= times[times.size() - 1])
    return values[values.size() - 1];
  int i = find_keyframe_index(times, t);
  float dt = times[i + 1] - times[i];
  float frac = (dt > 1e-8f) ? (t - times[i]) / dt : 0.0f;
  return vec3_lerp(values[i], values[i + 1], frac);
}

inline Quat sample_quat_channel(const std::vector<float> &times,
                                const std::vector<Quat> &values, float t) {
  if (values.empty())
    return {};
  if (values.size() == 1 || t <= times[0])
    return values[0];
  if (t >= times[times.size() - 1])
    return values[values.size() - 1];
  int i = find_keyframe_index(times, t);
  float dt = times[i + 1] - times[i];
  float frac = (dt > 1e-8f) ? (t - times[i]) / dt : 0.0f;
  return quat_slerp(values[i], values[i + 1], frac);
}

inline NodePose sample_node(const NodeChannels &ch, float t) {
  NodePose pose;
  if (!ch.trans_times.empty())
    pose.translation = sample_vec3_channel(ch.trans_times, ch.trans_values, t);
  if (!ch.rot_times.empty())
    pose.rotation = sample_quat_channel(ch.rot_times, ch.rot_values, t);
  if (!ch.scale_times.empty())
    pose.scale = sample_vec3_channel(ch.scale_times, ch.scale_values, t);
  else
    pose.scale = {1, 1, 1};
  return pose;
}

#endif // ANIMATION_H
