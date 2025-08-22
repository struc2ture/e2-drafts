#pragma once

#include "../common/types.h"

void ui_renderer_init();
void ui_renderer_submit_quad(v2 a, v2 b, v2 c, v2 d, v4 color);
void ui_renderer_submit_circle(v2 p, f32 r, v4 color);
void ui_renderer_draw(v2 window_size);
