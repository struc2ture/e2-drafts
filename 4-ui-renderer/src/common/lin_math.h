#pragma once

#include "types.h"

/*
 * OpenGL expects column-major.
 * m[column][row]
 * m[column * 4 + row]
 * M00 M01 M02 M03
 * M10 M11 M12 M13
 * M20 M21 M22 M23
 * M30 M31 M32 M33
 *
 * Laid out in memory like this:
 * M00 M10 M20 M30 M01 M11 ...
 */
typedef struct m4
{
    f32 d[16];
} m4;

m4 m4_identity();
m4 m4_proj_ortho(f32 left, f32 right, f32 bottom, f32 top, f32 near, f32 far);

static inline v2 v2_add(v2 a, v2 b)
{
    return V2(a.x + b.x, a.y + b.y);
}

static inline v2 v2_sub(v2 a, v2 b)
{
    return V2(a.x - b.x, a.y - b.y);
}
