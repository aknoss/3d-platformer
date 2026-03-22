#include "renderer.h"
#include <SDL3/SDL.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

static const char *VERT_SRC = R"(
#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aColor;
uniform mat4 uMVP;
out vec3 vColor;
void main() {
    gl_Position = uMVP * vec4(aPos, 1.0);
    vColor = aColor;
}
)";

static const char *FRAG_SRC = R"(
#version 330 core
in vec3 vColor;
out vec4 FragColor;
void main() {
    FragColor = vec4(vColor, 1.0);
}
)";

static const char *TEX_VERT_SRC = R"(
#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aUV;
uniform mat4 uMVP;
out vec2 vUV;
out vec3 vNormal;
void main() {
    gl_Position = uMVP * vec4(aPos, 1.0);
    vUV = aUV;
    vNormal = aNormal;
}
)";

static const char *TEX_FRAG_SRC = R"(
#version 330 core
in vec2 vUV;
in vec3 vNormal;
out vec4 FragColor;
uniform sampler2D uTexture;
void main() {
    vec3 lightDir = normalize(vec3(0.3, 1.0, 0.5));
    float diff = max(dot(normalize(vNormal), lightDir), 0.0);
    float light = 0.4 + 0.6 * diff;
    vec4 texColor = texture(uTexture, vUV);
    FragColor = vec4(texColor.rgb * light, texColor.a);
}
)";

static GLuint compile_shader(GLenum type, const char *src) {
  GLuint s = glCreateShader(type);
  glShaderSource(s, 1, &src, nullptr);
  glCompileShader(s);
  GLint ok;
  glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
  if (!ok) {
    char log[512];
    glGetShaderInfoLog(s, 512, nullptr, log);
    SDL_Log("Shader compile error: %s", log);
  }
  return s;
}

static GLuint link_program(GLuint vs, GLuint fs) {
  GLuint program = glCreateProgram();
  glAttachShader(program, vs);
  glAttachShader(program, fs);
  glLinkProgram(program);
  GLint ok;
  glGetProgramiv(program, GL_LINK_STATUS, &ok);
  if (!ok) {
    char log[512];
    glGetProgramInfoLog(program, 512, nullptr, log);
    SDL_Log("Program link error: %s", log);
  }
  glDeleteShader(vs);
  glDeleteShader(fs);
  return program;
}

ShaderProgram ShaderProgram::create_default() {
  GLuint vs = compile_shader(GL_VERTEX_SHADER, VERT_SRC);
  GLuint fs = compile_shader(GL_FRAGMENT_SHADER, FRAG_SRC);
  GLuint program = link_program(vs, fs);
  GLint mvp_loc = glGetUniformLocation(program, "uMVP");
  return ShaderProgram(program, mvp_loc);
}

ShaderProgram ShaderProgram::create_textured() {
  GLuint vs = compile_shader(GL_VERTEX_SHADER, TEX_VERT_SRC);
  GLuint fs = compile_shader(GL_FRAGMENT_SHADER, TEX_FRAG_SRC);
  GLuint program = link_program(vs, fs);
  GLint mvp_loc = glGetUniformLocation(program, "uMVP");
  GLint tex_loc = glGetUniformLocation(program, "uTexture");
  glUseProgram(program);
  glUniform1i(tex_loc, 0);
  return ShaderProgram(program, mvp_loc, tex_loc);
}

GLuint load_texture_from_memory(const unsigned char *data, unsigned int len) {
  int w, h, channels;
  stbi_set_flip_vertically_on_load(0);
  unsigned char *pixels =
      stbi_load_from_memory(data, (int)len, &w, &h, &channels, 4);
  if (!pixels) {
    SDL_Log("Failed to load texture from memory");
    return 0;
  }

  GLuint tex;
  glGenTextures(1, &tex);
  glBindTexture(GL_TEXTURE_2D, tex);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE,
               pixels);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

  stbi_image_free(pixels);
  return tex;
}
