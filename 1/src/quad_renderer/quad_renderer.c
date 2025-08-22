#include "quad_renderer.h"

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

struct Quad
{
    struct Vert p[4];
};

#define MAX_QUADS 1024
#define MAX_INDICES (MAX_QUADS * 6)
#define VERT_SIZE (sizeof(struct Vert))
#define QUAD_SIZE (sizeof(struct Quad))
#define QUAD_BUF_SIZE (MAX_QUADS * QUAD_SIZE)
#define IND_SIZE (sizeof(ind_buf[0]))
#define IND_BUF_SIZE (MAX_INDICES * IND_SIZE)

static GLuint vao = 0;
static GLuint vbo = 0;
static GLuint ebo = 0;
static GLuint shader_program = 0;

static struct Quad quad_buf[MAX_QUADS];
static size_t quad_count = 0;

static u32 ind_buf[MAX_INDICES];
static size_t ind_count = 0;

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

static void _add_indices(u32 base, u32 *indices, int count)
{
    for (int i = 0; i < count; i++)
    {
        ind_buf[ind_count++] = base + indices[i];
    }
}

void quad_renderer_init()
{
    shader_program = glg__create_shader_program(vs_src, fs_src);

    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);
    
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, QUAD_BUF_SIZE, NULL, GL_DYNAMIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, QUAD_BUF_SIZE, NULL, GL_DYNAMIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, VERT_SIZE, (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, VERT_SIZE, (void*)offsetof(struct Vert, color));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
    // glBindBuffer(GL_ARRAY_BUFFER, 0);
    // glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void quad_renderer_submit_quad(v2 a, v2 b, v2 c, v2 d, v4 color)
{
    if (quad_count >= MAX_QUADS)
    {
        warning("Max quad_count reached");
        return;
    }

    int ind_base = quad_count * 4;

    quad_buf[quad_count++] = (struct Quad){
        (struct Vert){a, color},
        (struct Vert){b, color},
        (struct Vert){c, color},
        (struct Vert){d, color},
    };

    _add_indices(ind_base, (u32[]){0, 1, 2, 0, 2, 3}, 6);
}

void quad_renderer_draw()
{
    glUseProgram(shader_program);

    glBindVertexArray(vao);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, QUAD_BUF_SIZE, NULL, GL_DYNAMIC_DRAW); // orphan
    glBufferSubData(GL_ARRAY_BUFFER, 0, quad_count * QUAD_SIZE, quad_buf);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, IND_BUF_SIZE, NULL, GL_DYNAMIC_DRAW); // orphan
    glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, ind_count * IND_SIZE, ind_buf);

    glDrawElements(GL_TRIANGLES, ind_count, GL_UNSIGNED_INT, 0);
    quad_count = 0;
    ind_count = 0;
}
