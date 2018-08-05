#include <cassert>
#include <cstdio>
#include <cstring>
#include <GL/glew.h>
#include "gl_util.h"
#include <vector>

#define NO_SDL_GLEXT
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>

const char textured_quad_vert_source[] =
#include "quad.vert"
;
const char textured_quad_frag_source[] =
#include "quad.frag"
;

const int width = 512;
const int height = 512;
GLuint quad_program;
GLuint vert_pos_attrib = 0;
GLuint vertex_array;
GLuint texture_attrib;
char* bitmap;

GLuint CompileShader(const char* shader_source, GLenum shader_type) {
  GLuint shader = glCreateShader(shader_type);
  glShaderSource(shader, 1, &shader_source, NULL);
  glCompileShader(shader);
  PrintShaderLog(shader);
  return shader;
}

GLuint CompileProgram(const char* vert_source, const char* frag_source) {
  GLuint program = glCreateProgram();
  glAttachShader(program, CompileShader(vert_source, GL_VERTEX_SHADER));
  glAttachShader(program, CompileShader(frag_source, GL_FRAGMENT_SHADER));
  glLinkProgram(program);
  PrintProgramLog(program);
  return program;
}

void GLAPIENTRY DebugCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam) {
  fprintf(stderr, "Deubg callback:  %s type = 0x%x, severity = 0x%x, message= %s\n", type == GL_DEBUG_TYPE_ERROR ? "** GL_ERROR **" : "", type, severity, message);
  assert(type != GL_DEBUG_TYPE_ERROR);
}

void InitGL() {
#ifdef DEBUG
  glEnable(GL_DEBUG_OUTPUT);
  glDebugMessageCallback(DebugCallback, nullptr);
#endif

  quad_program = CompileProgram(textured_quad_vert_source, textured_quad_frag_source);

  glClearColor(0, 0, 0, 1);

  /*
  std::vector<float> vertex_data {
    -.5, -.5,
     .5, -.5,
     .5,  .5,

    -.5, -.5,
     .5,  .5,
    -.5,  .5,
  };*/
  std::vector<float> vertex_data {
      0,   0,
     .5,   0,
     .5,  .5,

      0,   0,
     .5,  .5,
      0,  .5,
  };

  glGenVertexArrays(1, &vertex_array);
  glBindVertexArray(vertex_array);
  GLuint vbo;

  glGenBuffers(1, &vbo);
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBufferData(GL_ARRAY_BUFFER, vertex_data.size() * sizeof(float), vertex_data.data(), GL_STATIC_DRAW);

  glEnableVertexAttribArray(vert_pos_attrib);
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);

  GLuint texture_uniform = glGetUniformLocation(quad_program, "sampler");
  glUseProgram(quad_program);
  glUniform1i(texture_uniform, 0);

  GLuint texture;
  glActiveTexture(GL_TEXTURE0);
  glGenTextures(1, &texture);
  glBindTexture(GL_TEXTURE_2D, texture);
  bitmap = new char[width*height*4];
  memset(bitmap, 128, width*height*4);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, bitmap);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
}

void RenderBitmap(void* map, int width, int height) {
  glClear(GL_COLOR_BUFFER_BIT);
  glUseProgram(quad_program);
  glBindVertexArray(vertex_array);
  glEnableVertexAttribArray(vert_pos_attrib);
  glDrawArrays(GL_TRIANGLES, 0, 6);
}

int main() {
  SDL_Init(SDL_INIT_VIDEO);

  auto main_window = SDL_CreateWindow("Sad!", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 512, 512, SDL_WINDOW_OPENGL);

  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

  SDL_GL_CreateContext(main_window);

  assert(glewInit() == GLEW_OK);

  InitGL();

  SDL_Event e;
  while (true) {
    while(SDL_PollEvent(&e) != 0) {
      if(e.type == SDL_QUIT) {
        return 0;
      }
    }

    RenderBitmap(bitmap, width, height);
    SDL_GL_SwapWindow(main_window);
  }
}
