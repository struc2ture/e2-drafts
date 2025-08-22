#pragma once

#include <GLFW/glfw3.h>

#include "types.h"

GLFWwindow *glfwg__init(int window_w, int window_h);
v2i glfwg__get_fb_size();
