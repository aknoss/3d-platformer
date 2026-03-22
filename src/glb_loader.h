#ifndef GLB_LOADER_H
#define GLB_LOADER_H

#define CGLTF_IMPLEMENTATION
#include <cgltf.h>

#include "animation.h"
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

struct GlbVertex {
  float x, y, z;
  float nx, ny, nz;
  float u, v;
};

struct GlbModel {
  std::vector<GlbVertex> vertices;
};

// 4x4 column-major transform helpers for GLB loading
struct GlbMat4 {
  float m[16];
};

inline GlbMat4 glb_mat4_identity() {
  GlbMat4 r = {};
  r.m[0] = r.m[5] = r.m[10] = r.m[15] = 1.0f;
  return r;
}

inline GlbMat4 glb_mat4_mul(const GlbMat4 &a, const GlbMat4 &b) {
  GlbMat4 r = {};
  for (int c = 0; c < 4; c++)
    for (int row = 0; row < 4; row++)
      for (int k = 0; k < 4; k++)
        r.m[c * 4 + row] += a.m[k * 4 + row] * b.m[c * 4 + k];
  return r;
}

inline GlbMat4 glb_node_local_transform(const cgltf_node *node) {
  GlbMat4 local;
  if (node->has_matrix) {
    for (int i = 0; i < 16; i++)
      local.m[i] = node->matrix[i];
  } else {
    // Build TRS
    float tx = 0, ty = 0, tz = 0;
    float qx = 0, qy = 0, qz = 0, qw = 1;
    float sx = 1, sy = 1, sz = 1;
    if (node->has_translation) {
      tx = node->translation[0];
      ty = node->translation[1];
      tz = node->translation[2];
    }
    if (node->has_rotation) {
      qx = node->rotation[0];
      qy = node->rotation[1];
      qz = node->rotation[2];
      qw = node->rotation[3];
    }
    if (node->has_scale) {
      sx = node->scale[0];
      sy = node->scale[1];
      sz = node->scale[2];
    }
    // Rotation matrix from quaternion, scaled
    float xx = qx * qx, yy = qy * qy, zz = qz * qz;
    float xy = qx * qy, xz = qx * qz, yz = qy * qz;
    float wx = qw * qx, wy = qw * qy, wz = qw * qz;
    local.m[0] = (1 - 2 * (yy + zz)) * sx;
    local.m[1] = 2 * (xy + wz) * sx;
    local.m[2] = 2 * (xz - wy) * sx;
    local.m[3] = 0;
    local.m[4] = 2 * (xy - wz) * sy;
    local.m[5] = (1 - 2 * (xx + zz)) * sy;
    local.m[6] = 2 * (yz + wx) * sy;
    local.m[7] = 0;
    local.m[8] = 2 * (xz + wy) * sz;
    local.m[9] = 2 * (yz - wx) * sz;
    local.m[10] = (1 - 2 * (xx + yy)) * sz;
    local.m[11] = 0;
    local.m[12] = tx;
    local.m[13] = ty;
    local.m[14] = tz;
    local.m[15] = 1;
  }
  return local;
}

inline void glb_transform_vertex(const GlbMat4 &m, GlbVertex &v) {
  float px = m.m[0] * v.x + m.m[4] * v.y + m.m[8] * v.z + m.m[12];
  float py = m.m[1] * v.x + m.m[5] * v.y + m.m[9] * v.z + m.m[13];
  float pz = m.m[2] * v.x + m.m[6] * v.y + m.m[10] * v.z + m.m[14];
  v.x = px;
  v.y = py;
  v.z = pz;
  // Transform normal (upper-left 3x3, no translation)
  float nx = m.m[0] * v.nx + m.m[4] * v.ny + m.m[8] * v.nz;
  float ny = m.m[1] * v.nx + m.m[5] * v.ny + m.m[9] * v.nz;
  float nz = m.m[2] * v.nx + m.m[6] * v.ny + m.m[10] * v.nz;
  float len = sqrtf(nx * nx + ny * ny + nz * nz);
  if (len > 0.0001f) {
    v.nx = nx / len;
    v.ny = ny / len;
    v.nz = nz / len;
  }
}

inline void glb_collect_node(const cgltf_node *node, const GlbMat4 &parent,
                             GlbModel &model) {
  GlbMat4 world = glb_mat4_mul(parent, glb_node_local_transform(node));

  if (node->mesh) {
    cgltf_mesh *mesh = node->mesh;
    for (cgltf_size pi = 0; pi < mesh->primitives_count; pi++) {
      cgltf_primitive *prim = &mesh->primitives[pi];

      cgltf_accessor *pos_acc = nullptr;
      cgltf_accessor *norm_acc = nullptr;
      cgltf_accessor *uv_acc = nullptr;

      for (cgltf_size ai = 0; ai < prim->attributes_count; ai++) {
        if (prim->attributes[ai].type == cgltf_attribute_type_position)
          pos_acc = prim->attributes[ai].data;
        else if (prim->attributes[ai].type == cgltf_attribute_type_normal)
          norm_acc = prim->attributes[ai].data;
        else if (prim->attributes[ai].type == cgltf_attribute_type_texcoord)
          uv_acc = prim->attributes[ai].data;
      }

      if (!pos_acc)
        continue;

      auto read_vert = [&](cgltf_size vi) {
        GlbVertex vert = {};
        cgltf_accessor_read_float(pos_acc, vi, &vert.x, 3);
        if (norm_acc)
          cgltf_accessor_read_float(norm_acc, vi, &vert.nx, 3);
        if (uv_acc)
          cgltf_accessor_read_float(uv_acc, vi, &vert.u, 2);
        glb_transform_vertex(world, vert);
        model.vertices.push_back(vert);
      };

      if (prim->indices) {
        cgltf_accessor *idx_acc = prim->indices;
        for (cgltf_size i = 0; i < idx_acc->count; i++)
          read_vert(cgltf_accessor_read_index(idx_acc, i));
      } else {
        for (cgltf_size i = 0; i < pos_acc->count; i++)
          read_vert(i);
      }
    }
  }

  for (cgltf_size ci = 0; ci < node->children_count; ci++)
    glb_collect_node(node->children[ci], world, model);
}

// Rest-pose TRS for any node (including non-mesh nodes like "root")
struct NodeRestPose {
  std::string name;
  Vec3 translation;
  Quat rotation;
  Vec3 scale;
  std::string parent_name;
  NodeRestPose() : translation(0, 0, 0), rotation(), scale(1, 1, 1) {}
};

struct GlbPart {
  std::string name;
  std::vector<GlbVertex> vertices; // in local mesh space (not world-baked)
  std::string parent_name;
};

struct GlbPartsModel {
  std::vector<GlbPart> parts;
  std::vector<NodeRestPose> node_rest_poses; // all nodes including non-mesh
};

struct GlbLoadResult {
  GlbPartsModel parts;
  AnimSet animations;
};

inline void glb_collect_part(const cgltf_node *node,
                             const cgltf_node *parent_node,
                             GlbPartsModel &model) {
  // Store rest pose for every named node (mesh or not)
  if (node->name) {
    NodeRestPose rp;
    rp.name = node->name;
    rp.translation = node->has_translation
                         ? Vec3(node->translation[0], node->translation[1],
                                node->translation[2])
                         : Vec3(0, 0, 0);
    rp.rotation = node->has_rotation
                      ? Quat(node->rotation[0], node->rotation[1],
                             node->rotation[2], node->rotation[3])
                      : Quat();
    rp.scale = node->has_scale
                   ? Vec3(node->scale[0], node->scale[1], node->scale[2])
                   : Vec3(1, 1, 1);
    rp.parent_name =
        (parent_node && parent_node->name) ? parent_node->name : "";
    model.node_rest_poses.push_back(std::move(rp));
  }

  if (node->mesh) {
    GlbPart part;
    part.name = node->name ? node->name : "";
    part.parent_name =
        (parent_node && parent_node->name) ? parent_node->name : "";

    cgltf_mesh *mesh = node->mesh;
    for (cgltf_size pi = 0; pi < mesh->primitives_count; pi++) {
      cgltf_primitive *prim = &mesh->primitives[pi];
      cgltf_accessor *pos_acc = nullptr;
      cgltf_accessor *norm_acc = nullptr;
      cgltf_accessor *uv_acc = nullptr;

      for (cgltf_size ai = 0; ai < prim->attributes_count; ai++) {
        if (prim->attributes[ai].type == cgltf_attribute_type_position)
          pos_acc = prim->attributes[ai].data;
        else if (prim->attributes[ai].type == cgltf_attribute_type_normal)
          norm_acc = prim->attributes[ai].data;
        else if (prim->attributes[ai].type == cgltf_attribute_type_texcoord)
          uv_acc = prim->attributes[ai].data;
      }
      if (!pos_acc)
        continue;

      auto read_vert = [&](cgltf_size vi) {
        GlbVertex vert = {};
        cgltf_accessor_read_float(pos_acc, vi, &vert.x, 3);
        if (norm_acc)
          cgltf_accessor_read_float(norm_acc, vi, &vert.nx, 3);
        if (uv_acc)
          cgltf_accessor_read_float(uv_acc, vi, &vert.u, 2);
        // Vertices stay in local mesh space — no world transform baking
        part.vertices.push_back(vert);
      };

      if (prim->indices) {
        cgltf_accessor *idx_acc = prim->indices;
        for (cgltf_size i = 0; i < idx_acc->count; i++)
          read_vert(cgltf_accessor_read_index(idx_acc, i));
      } else {
        for (cgltf_size i = 0; i < pos_acc->count; i++)
          read_vert(i);
      }
    }
    model.parts.push_back(std::move(part));
  }

  for (cgltf_size ci = 0; ci < node->children_count; ci++)
    glb_collect_part(node->children[ci], node, model);
}

inline GlbPartsModel load_glb_parts_from_memory(const unsigned char *data,
                                                unsigned int len) {
  GlbPartsModel model;

  cgltf_options options = {};
  cgltf_data *gltf = nullptr;
  cgltf_result result = cgltf_parse(&options, data, len, &gltf);
  if (result != cgltf_result_success) {
    fprintf(stderr, "GLB parse failed: %d\n", result);
    return model;
  }

  result = cgltf_load_buffers(&options, gltf, nullptr);
  if (result != cgltf_result_success) {
    fprintf(stderr, "GLB buffer load failed: %d\n", result);
    cgltf_free(gltf);
    return model;
  }

  for (cgltf_size si = 0; si < gltf->scenes_count; si++) {
    cgltf_scene *scene = &gltf->scenes[si];
    for (cgltf_size ni = 0; ni < scene->nodes_count; ni++)
      glb_collect_part(scene->nodes[ni], nullptr, model);
  }

  cgltf_free(gltf);
  return model;
}

// Extract animation clips from parsed glTF data
inline AnimSet glb_extract_animations(const cgltf_data *gltf) {
  AnimSet anim_set;

  for (cgltf_size ai = 0; ai < gltf->animations_count; ai++) {
    const cgltf_animation *anim = &gltf->animations[ai];
    AnimClip clip;
    clip.name = anim->name ? anim->name : "";
    clip.duration = 0;

    for (cgltf_size ci = 0; ci < anim->channels_count; ci++) {
      const cgltf_animation_channel *ch = &anim->channels[ci];
      if (!ch->target_node || !ch->target_node->name)
        continue;

      std::string node_name = ch->target_node->name;
      const cgltf_animation_sampler *sampler = ch->sampler;

      // Read keyframe times
      std::vector<float> times(sampler->input->count);
      for (cgltf_size i = 0; i < sampler->input->count; i++)
        cgltf_accessor_read_float(sampler->input, i, &times[i], 1);

      // Track max time for clip duration
      if (!times.empty() && times.back() > clip.duration)
        clip.duration = times.back();

      // Find or create NodeChannels for this node
      NodeChannels *nc = nullptr;
      for (auto &existing : clip.channels) {
        if (existing.node_name == node_name) {
          nc = &existing;
          break;
        }
      }
      if (!nc) {
        clip.channels.push_back({});
        nc = &clip.channels.back();
        nc->node_name = node_name;
      }

      // Read keyframe values based on channel target
      if (ch->target_path == cgltf_animation_path_type_translation) {
        nc->trans_times = times;
        nc->trans_values.resize(sampler->output->count);
        for (cgltf_size i = 0; i < sampler->output->count; i++)
          cgltf_accessor_read_float(sampler->output, i, &nc->trans_values[i].x,
                                    3);
      } else if (ch->target_path == cgltf_animation_path_type_rotation) {
        nc->rot_times = times;
        nc->rot_values.resize(sampler->output->count);
        for (cgltf_size i = 0; i < sampler->output->count; i++) {
          float q[4];
          cgltf_accessor_read_float(sampler->output, i, q, 4);
          nc->rot_values[i] = {q[0], q[1], q[2], q[3]};
        }
      } else if (ch->target_path == cgltf_animation_path_type_scale) {
        nc->scale_times = times;
        nc->scale_values.resize(sampler->output->count);
        for (cgltf_size i = 0; i < sampler->output->count; i++)
          cgltf_accessor_read_float(sampler->output, i, &nc->scale_values[i].x,
                                    3);
      }
    }

    anim_set.clips.push_back(std::move(clip));
  }

  return anim_set;
}

// Combined load: extracts both mesh parts and animation clips
inline GlbLoadResult load_glb_with_animations(const unsigned char *data,
                                              unsigned int len) {
  GlbLoadResult result;

  cgltf_options options = {};
  cgltf_data *gltf = nullptr;
  cgltf_result res = cgltf_parse(&options, data, len, &gltf);
  if (res != cgltf_result_success) {
    fprintf(stderr, "GLB parse failed: %d\n", res);
    return result;
  }

  res = cgltf_load_buffers(&options, gltf, nullptr);
  if (res != cgltf_result_success) {
    fprintf(stderr, "GLB buffer load failed: %d\n", res);
    cgltf_free(gltf);
    return result;
  }

  // Extract mesh parts
  for (cgltf_size si = 0; si < gltf->scenes_count; si++) {
    cgltf_scene *scene = &gltf->scenes[si];
    for (cgltf_size ni = 0; ni < scene->nodes_count; ni++)
      glb_collect_part(scene->nodes[ni], nullptr, result.parts);
  }

  // Extract animations
  result.animations = glb_extract_animations(gltf);

  cgltf_free(gltf);
  return result;
}

inline GlbModel load_glb_from_memory(const unsigned char *data,
                                     unsigned int len) {
  GlbModel model;

  cgltf_options options = {};
  cgltf_data *gltf = nullptr;
  cgltf_result result = cgltf_parse(&options, data, len, &gltf);
  if (result != cgltf_result_success) {
    fprintf(stderr, "GLB parse failed: %d\n", result);
    return model;
  }

  result = cgltf_load_buffers(&options, gltf, nullptr);
  if (result != cgltf_result_success) {
    fprintf(stderr, "GLB buffer load failed: %d\n", result);
    cgltf_free(gltf);
    return model;
  }

  // Walk scene nodes to apply transforms
  GlbMat4 identity = glb_mat4_identity();
  for (cgltf_size si = 0; si < gltf->scenes_count; si++) {
    cgltf_scene *scene = &gltf->scenes[si];
    for (cgltf_size ni = 0; ni < scene->nodes_count; ni++)
      glb_collect_node(scene->nodes[ni], identity, model);
  }

  cgltf_free(gltf);
  return model;
}

#endif // GLB_LOADER_H
