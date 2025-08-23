#pragma once

#include "../common/types.h"

void text_renderer_init();
void text_renderer_submit_string(const char *str, v2 p, v4 color);
void text_renderer_draw(v2 window_size);
