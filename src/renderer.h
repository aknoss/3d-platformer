#ifndef RENDERER_H
#define RENDERER_H
#include "math.h"
#include <vector>

#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#include <GL/glext.h>

class Mesh {
public:
  Mesh() : vao_(0), vbo_(0), vertex_count_(0) {}

  explicit Mesh(const std::vector<float> &data) {
    vertex_count_ = (int)data.size() / 6;
    glGenVertexArrays(1, &vao_);
    glBindVertexArray(vao_);
    glGenBuffers(1, &vbo_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(data.size() * sizeof(float)),
                 data.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float),
                          (void *)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float),
                          (void *)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);
  }

  ~Mesh() {
    if (vbo_ != 0)
      glDeleteBuffers(1, &vbo_);
    if (vao_ != 0)
      glDeleteVertexArrays(1, &vao_);
  }

  Mesh(const Mesh &) = delete;
  Mesh &operator=(const Mesh &) = delete;

  Mesh(Mesh &&o) noexcept
      : vao_(o.vao_), vbo_(o.vbo_), vertex_count_(o.vertex_count_) {
    o.vao_ = 0;
    o.vbo_ = 0;
    o.vertex_count_ = 0;
  }

  Mesh &operator=(Mesh &&o) noexcept {
    if (this != &o) {
      if (vbo_ != 0)
        glDeleteBuffers(1, &vbo_);
      if (vao_ != 0)
        glDeleteVertexArrays(1, &vao_);
      vao_ = o.vao_;
      vbo_ = o.vbo_;
      vertex_count_ = o.vertex_count_;
      o.vao_ = 0;
      o.vbo_ = 0;
      o.vertex_count_ = 0;
    }
    return *this;
  }

  void draw() const {
    glBindVertexArray(vao_);
    glDrawArrays(GL_TRIANGLES, 0, vertex_count_);
  }

  bool valid() const { return vao_ != 0; }

private:
  GLuint vao_, vbo_;
  int vertex_count_;
};

// Vertex format: pos(3) + normal(3) + uv(2) = 8 floats
class TexturedMesh {
public:
  TexturedMesh() : vao_(0), vbo_(0), tex_(0), vertex_count_(0) {}

  TexturedMesh(const std::vector<float> &data, GLuint texture) : tex_(texture) {
    vertex_count_ = (int)data.size() / 8;
    glGenVertexArrays(1, &vao_);
    glBindVertexArray(vao_);
    glGenBuffers(1, &vbo_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(data.size() * sizeof(float)),
                 data.data(), GL_STATIC_DRAW);
    // position
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float),
                          (void *)0);
    glEnableVertexAttribArray(0);
    // normal
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float),
                          (void *)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    // uv
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float),
                          (void *)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glBindVertexArray(0);
  }

  ~TexturedMesh() {
    if (vbo_ != 0)
      glDeleteBuffers(1, &vbo_);
    if (vao_ != 0)
      glDeleteVertexArrays(1, &vao_);
    if (tex_ != 0)
      glDeleteTextures(1, &tex_);
  }

  TexturedMesh(const TexturedMesh &) = delete;
  TexturedMesh &operator=(const TexturedMesh &) = delete;

  TexturedMesh(TexturedMesh &&o) noexcept
      : vao_(o.vao_), vbo_(o.vbo_), tex_(o.tex_),
        vertex_count_(o.vertex_count_) {
    o.vao_ = 0;
    o.vbo_ = 0;
    o.tex_ = 0;
    o.vertex_count_ = 0;
  }

  TexturedMesh &operator=(TexturedMesh &&o) noexcept {
    if (this != &o) {
      if (vbo_ != 0)
        glDeleteBuffers(1, &vbo_);
      if (vao_ != 0)
        glDeleteVertexArrays(1, &vao_);
      if (tex_ != 0)
        glDeleteTextures(1, &tex_);
      vao_ = o.vao_;
      vbo_ = o.vbo_;
      tex_ = o.tex_;
      vertex_count_ = o.vertex_count_;
      o.vao_ = 0;
      o.vbo_ = 0;
      o.tex_ = 0;
      o.vertex_count_ = 0;
    }
    return *this;
  }

  void draw() const {
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex_);
    glBindVertexArray(vao_);
    glDrawArrays(GL_TRIANGLES, 0, vertex_count_);
  }

  bool valid() const { return vao_ != 0; }

private:
  GLuint vao_, vbo_, tex_;
  int vertex_count_;
};

GLuint load_texture(const char *path);

class ShaderProgram {
public:
  ShaderProgram() : program_(0), mvp_loc_(-1), tex_loc_(-1) {}

  ~ShaderProgram() {
    if (program_ != 0)
      glDeleteProgram(program_);
  }

  ShaderProgram(const ShaderProgram &) = delete;
  ShaderProgram &operator=(const ShaderProgram &) = delete;

  ShaderProgram(ShaderProgram &&o) noexcept
      : program_(o.program_), mvp_loc_(o.mvp_loc_), tex_loc_(o.tex_loc_) {
    o.program_ = 0;
    o.mvp_loc_ = -1;
    o.tex_loc_ = -1;
  }

  ShaderProgram &operator=(ShaderProgram &&o) noexcept {
    if (this != &o) {
      if (program_ != 0)
        glDeleteProgram(program_);
      program_ = o.program_;
      mvp_loc_ = o.mvp_loc_;
      tex_loc_ = o.tex_loc_;
      o.program_ = 0;
      o.mvp_loc_ = -1;
      o.tex_loc_ = -1;
    }
    return *this;
  }

  static ShaderProgram create_default();
  static ShaderProgram create_textured();

  void use() const { glUseProgram(program_); }
  void set_mvp(const Mat4 &mvp) const {
    glUniformMatrix4fv(mvp_loc_, 1, GL_FALSE, mvp.m);
  }

private:
  ShaderProgram(GLuint program, GLint mvp_loc, GLint tex_loc = -1)
      : program_(program), mvp_loc_(mvp_loc), tex_loc_(tex_loc) {}

  GLuint program_;
  GLint mvp_loc_;
  GLint tex_loc_;
};

#endif // RENDERER_H
