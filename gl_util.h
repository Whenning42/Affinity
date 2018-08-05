#include <cassert>
#include <cstdio>
#include "GL/glew.h"

inline void PrintProgramLog(GLuint program) {
  assert(glIsProgram(program));

  int log_length = 0;
  glGetProgramiv(program, GL_INFO_LOG_LENGTH, &log_length);
  if(!log_length) return;

  char* info_log = new char[log_length];
  glGetProgramInfoLog(program, log_length, nullptr, info_log);
  printf("Program Log: %d\n", program);
  printf("%s\n", info_log);
  delete info_log;
}

inline void PrintShaderLog(GLuint shader) {
  assert(glIsShader(shader));

  int log_length = 0;
  glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &log_length);
  if(!log_length) return;

  char* info_log = new char[log_length];
  glGetShaderInfoLog(shader, log_length, nullptr, info_log);
  printf("Shader Log: %d\n", shader);
  printf("%s\n", info_log);
  delete info_log;
}

/*
void PrintGLError() {
  GLenum error = glGetError();
  if(error == GL_INVALID_ENUM) printf("GL error: GL_INVALID_ENUM\n");
  else if(error == GL_INVALID_VALUE) printf("GL error: GL_INVALID_VALUE\n");
  else if(error == GL_INVALID_OPERATION) printf("GL error: GL_INVALID_OPERATION\n");
  else if(error == GL_INVALID_FRAMEBUFFER_OPERATION) printf("GL error: GL_INVALID_FRAMEBUFFER_OPERATION\n");
  else if(error == GL_OUT_OF_MEMORY) printf("GL error: GL_OUT_OF_MEMORY\n");
  else if(error == GL_STACK_UNDERFLOW) printf("GL error: GL_STACK_UNDERFLOW\n");
  else if(error == GL_STACK_OVERFLOW) printf("GL error: GL_STACK_OVERFLOW\n");
  assert(error == GL_NO_ERROR);
}*/
