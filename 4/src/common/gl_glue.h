#pragma once

#include <OpenGL/gl3.h>

#include "types.h"

void glg__set_viewport_size(v2i size);
void glg__clear(v4 c);

bool glg__check_compile_success(GLuint shader, const char *src);
bool glg__check_link_success(GLuint prog);
GLuint glg__create_shader_program(const char *vs_src, const char *fs_src);
