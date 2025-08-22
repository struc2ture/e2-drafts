#pragma once

#include "../common/types.h"

void quad_renderer_init();
void quad_renderer_submit_quad(v2 a, v2 b, v2 c, v2 d, v4 color);
void quad_renderer_draw();
