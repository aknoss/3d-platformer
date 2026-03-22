#ifndef GLB_LOADER_H
#define GLB_LOADER_H

#define CGLTF_IMPLEMENTATION
#include <cgltf.h>

#include <cmath>
#include <cstdio>
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

struct GlbPart {
  std::string name;
  std::vector<GlbVertex> vertices;
  float pivot_x, pivot_y, pivot_z; // translation relative to parent
};

struct GlbPartsModel {
  std::vector<GlbPart> parts;
};

inline void glb_collect_part(const cgltf_node *node, const GlbMat4 &parent,
                             GlbPartsModel &model) {
  GlbMat4 world = glb_mat4_mul(parent, glb_node_local_transform(node));

  if (node->mesh) {
    GlbPart part;
    part.name = node->name ? node->name : "";
    // Store world-space pivot (local translation transformed by parent)
    float lx = node->has_translation ? node->translation[0] : 0;
    float ly = node->has_translation ? node->translation[1] : 0;
    float lz = node->has_translation ? node->translation[2] : 0;
    part.pivot_x =
        parent.m[0] * lx + parent.m[4] * ly + parent.m[8] * lz + parent.m[12];
    part.pivot_y =
        parent.m[1] * lx + parent.m[5] * ly + parent.m[9] * lz + parent.m[13];
    part.pivot_z =
        parent.m[2] * lx + parent.m[6] * ly + parent.m[10] * lz + parent.m[14];

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
        // Transform by world matrix (bake full transform into vertices)
        glb_transform_vertex(world, vert);
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
    glb_collect_part(node->children[ci], world, model);
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

  GlbMat4 identity = glb_mat4_identity();
  for (cgltf_size si = 0; si < gltf->scenes_count; si++) {
    cgltf_scene *scene = &gltf->scenes[si];
    for (cgltf_size ni = 0; ni < scene->nodes_count; ni++)
      glb_collect_part(scene->nodes[ni], identity, model);
  }

  cgltf_free(gltf);
  return model;
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
