#pragma once

#include "../common/types.h"

void triangle_renderer_init();
void triangle_renderer_submit_triangle(v2 a, v2 b, v2 c, v4 color);
void triangle_renderer_draw();
