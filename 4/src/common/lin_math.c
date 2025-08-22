#include "lin_math.h"

#include "types.h"

m4 m4_identity()
{
    m4 m = {};
    m.d[0] = 1; m.d[4] = 0; m.d[ 8] = 0; m.d[12] = 0;
    m.d[1] = 0; m.d[5] = 1; m.d[ 9] = 0; m.d[13] = 0;
    m.d[2] = 0; m.d[6] = 0; m.d[10] = 1; m.d[14] = 0;
    m.d[3] = 0; m.d[7] = 0; m.d[11] = 0; m.d[15] = 1;
    return m;
}

m4 m4_proj_ortho(f32 left, f32 right, f32 bottom, f32 top, f32 near, f32 far)
{
    m4 m;
    f32 rl = right - left;
    f32 tb = top - bottom;
    f32 fn = far - near;

    m.d[0]  = 2.0f / rl;
    m.d[1]  = 0;
    m.d[2]  = 0;
    m.d[3]  = 0;

    m.d[4]  = 0;
    m.d[5]  = 2.0f / tb;
    m.d[6]  = 0;
    m.d[7]  = 0;

    m.d[8]  = 0;
    m.d[9]  = 0;
    m.d[10] = -2.0f / fn;
    m.d[11] = 0;

    m.d[12] = -(right + left) / rl;
    m.d[13] = -(top + bottom) / tb;
    m.d[14] = -(far + near) / fn;
    m.d[15] = 1.0f;

    return m;
}
