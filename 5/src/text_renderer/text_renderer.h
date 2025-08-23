#pragma once

#include "../common/types.h"

void text_renderer_init();
void text_renderer_submit_string(v2 a, v2 b, v2 c, v2 d, v4 color);
void text_renderer_draw(v2 window_size);
