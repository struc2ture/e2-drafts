#include "gl_glue.h"

#include <OpenGL/gl3.h>

#include "types.h"

void glg__set_viewport_size(v2i size)
{
    glViewport(0, 0, size.x, size.y);
}

void glg__clear(v4 c)
{
    glClearColor(c.r, c.g, c.b, c.a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}
