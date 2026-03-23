#ifndef GLB_LOADER_H
#define GLB_LOADER_H

#define CGLTF_IMPLEMENTATION
#include <cgltf.h>

#include "animation.h"
#include <cstdio>
#include <string>
#include <vector>

struct GlbVertex {
  float x, y, z;
  float nx, ny, nz;
  float u, v;
};

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

#endif // GLB_LOADER_H
