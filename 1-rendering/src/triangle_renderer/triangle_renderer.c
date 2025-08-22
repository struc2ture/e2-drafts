#include "triangle_renderer.h"

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include <OpenGL/gl3.h>

#include "../common/gl_glue.h"
#include "../common/types.h"
#include "../common/util.h"

struct Vert
{
    v2 pos;
    v4 color;
};

struct Tri
{
    struct Vert p[3];
};

#define MAX_TRIANGLES 1024
#define VERT_SIZE (sizeof(struct Vert))
#define TRI_SIZE (sizeof(struct Tri))
#define TRI_BUF_SIZE (MAX_TRIANGLES * TRI_SIZE)

static GLuint vao = 0;
static GLuint vbo = 0;
static GLuint shader_program = 0;

static struct Tri tri_buf[MAX_TRIANGLES];
static size_t tri_count = 0;

static const char* vs_src =
    "#version 330 core\n"
    "layout(location = 0) in vec2 inPos;\n"
    "layout(location = 1) in vec4 inColor;\n"
    "out vec4 Color;\n"
    "void main()\n"
    "{\n"
    "    gl_Position = vec4(inPos, 0.0, 1.0);\n"
    "    Color = inColor;\n"
    "}\n";

static const char* fs_src =
    "#version 330 core\n"
    "in vec4 Color;\n"
    "out vec4 FragColor;\n"
    "void main()\n"
    "{\n"
    "    FragColor = Color;\n"
    "}\n";

void triangle_renderer_init()
{
    shader_program = glg__create_shader_program(vs_src, fs_src);

    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, TRI_BUF_SIZE, NULL, GL_DYNAMIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, VERT_SIZE, (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, VERT_SIZE, (void*)offsetof(struct Vert, color));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
}

void triangle_renderer_submit_triangle(v2 a, v2 b, v2 c, v4 color)
{
    if (tri_count >= MAX_TRIANGLES)
    {
        warning("Max tri_count reached");
        return;
    }

    tri_buf[tri_count++] = (struct Tri){
        (struct Vert){a, color},
        (struct Vert){b, color},
        (struct Vert){c, color},
    };
}

void triangle_renderer_draw()
{
    glUseProgram(shader_program);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, TRI_BUF_SIZE, NULL, GL_DYNAMIC_DRAW); // orphan
    glBufferSubData(GL_ARRAY_BUFFER, 0, tri_count * TRI_SIZE, tri_buf);
    glDrawArrays(GL_TRIANGLES, 0, (GLsizei)(tri_count * 3));
    tri_count = 0;
}
