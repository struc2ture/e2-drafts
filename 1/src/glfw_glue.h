#pragma once

#include <GLFW/glfw3.h>

#include "types.h"

static GLFWwindow *glfwg_init(int window_w, int window_h);
static v2i glfwg_get_fb_size();
