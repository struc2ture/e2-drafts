#include <OpenGL/gl3.h>

#define DRAW__VERT_MAX 4096
#define DRAW__INDEX_MAX 8192

#include "types.h"

typedef
struct Draw_Vert
{
    f32 x, y;
    
} Draw_Vert;

typedef
struct Draw_VertBuf
{
    Draw_Vert verts[DRAW__VERT_MAX];
    int vert_count;

    u32 indices[DRAW__INDEX_MAX];
    int index_count;

    u32 vao, vbo, ebo;
} Draw_VertBuf;

static Draw_VertBuf *draw__g_VertBuf;

static void draw__init()
{
    draw__g_VertBuf = _draw__make_vb();
}

static void draw__quad()
{

}

static void draw__frame()
{
}