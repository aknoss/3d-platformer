#ifndef OBJ_LOADER_H
#define OBJ_LOADER_H
#pragma once

#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

struct ObjVertex {
  float x, y, z;
  float u, v;
  float nx, ny, nz;
};

struct ObjModel {
  std::vector<ObjVertex> vertices;
};

inline ObjModel load_obj_from_memory(const unsigned char *data,
                                     unsigned int len) {
  ObjModel model;

  std::vector<float> positions;
  std::vector<float> texcoords;
  std::vector<float> normals;

  const char *src = (const char *)data;
  const char *end = src + len;

  while (src < end) {
    // Extract one line
    const char *line_end = src;
    while (line_end < end && *line_end != '\n')
      line_end++;

    char line[256];
    int line_len = (int)(line_end - src);
    if (line_len >= (int)sizeof(line))
      line_len = (int)sizeof(line) - 1;
    memcpy(line, src, line_len);
    line[line_len] = '\0';

    src = line_end + 1;

    if (line[0] == 'v' && line[1] == ' ') {
      float x, y, z;
      sscanf(line + 2, "%f %f %f", &x, &y, &z);
      positions.push_back(x);
      positions.push_back(y);
      positions.push_back(z);
    } else if (line[0] == 'v' && line[1] == 't') {
      float u, v;
      sscanf(line + 3, "%f %f", &u, &v);
      texcoords.push_back(u);
      texcoords.push_back(v);
    } else if (line[0] == 'v' && line[1] == 'n') {
      float x, y, z;
      sscanf(line + 3, "%f %f %f", &x, &y, &z);
      normals.push_back(x);
      normals.push_back(y);
      normals.push_back(z);
    } else if (line[0] == 'f' && line[1] == ' ') {
      int vi[4], ti[4], ni[4];
      int count = 0;
      char *ptr = line + 2;
      while (count < 4 && *ptr) {
        while (*ptr == ' ')
          ptr++;
        if (*ptr == '\0' || *ptr == '\n')
          break;
        if (sscanf(ptr, "%d/%d/%d", &vi[count], &ti[count], &ni[count]) == 3) {
          count++;
        }
        while (*ptr && *ptr != ' ' && *ptr != '\n')
          ptr++;
      }

      for (int i = 0; i < count; i++) {
        vi[i]--;
        ti[i]--;
        ni[i]--;
      }

      auto make_vert = [&](int idx) -> ObjVertex {
        ObjVertex v = {};
        if (vi[idx] * 3 + 2 < (int)positions.size()) {
          v.x = positions[vi[idx] * 3];
          v.y = positions[vi[idx] * 3 + 1];
          v.z = positions[vi[idx] * 3 + 2];
        }
        if (ti[idx] * 2 + 1 < (int)texcoords.size()) {
          v.u = texcoords[ti[idx] * 2];
          v.v = texcoords[ti[idx] * 2 + 1];
        }
        if (ni[idx] * 3 + 2 < (int)normals.size()) {
          v.nx = normals[ni[idx] * 3];
          v.ny = normals[ni[idx] * 3 + 1];
          v.nz = normals[ni[idx] * 3 + 2];
        }
        return v;
      };

      for (int i = 1; i < count - 1; i++) {
        model.vertices.push_back(make_vert(0));
        model.vertices.push_back(make_vert(i));
        model.vertices.push_back(make_vert(i + 1));
      }
    }
  }

  return model;
}

#endif // OBJ_LOADER_H
