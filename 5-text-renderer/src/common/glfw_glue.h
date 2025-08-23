#pragma once

#include <GLFW/glfw3.h>

#include "types.h"

GLFWwindow *glfwg__init(int window_w, int window_h);
v2i glfwg__get_fb_size();
v2i glfwg__get_window_size();
v2 glfwg__get_window_size_f();
void glfwg__perform_timing_calculations();
f32 glfwg__get_delta_time();
f32 glfwg__get_avg_fps();
bool glfwg__is_key_pressed(int key);
