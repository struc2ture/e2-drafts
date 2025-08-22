#include "ui_renderer.h"

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include <OpenGL/gl3.h>

#include "../common/gl_glue.h"
#include "../common/lin_math.h"
#include "../common/types.h"
#include "../common/util.h"

#include <stb_image.h>

/*
 * Checklist when changing vert attribute layout
 * - vert struct
 * - layout(location = n) in shader
 * - glVertexAttribPointer and glEnableVertexAttribArray (double check the attrib index and element count!)
 */

struct Vert
{
    v2 pos;
    v2 tex_coord;
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

#define ATLAS_PATH "res/ui_atlas.png"
#define ATLAS_DIM 1024.0f
#define ATLAS_CELL_DIM 64.0f
#define ATLAS_CELL_PAD 4.0f

#define ATLAS_TEXTURE_UNIT 0
#define ATLAS_TEXTURE_UNIT_ENUM (GL_TEXTURE0 + ATLAS_TEXTURE_UNIT)

static GLuint vao = 0;
static GLuint vbo = 0;
static GLuint ebo = 0;
static GLuint shader_program = 0;
static GLint shader_loc_uMvp = 0;
static GLint shader_loc_uTex = 0;

static struct Quad quad_buf[MAX_QUADS];
static size_t quad_count = 0;

static u32 ind_buf[MAX_INDICES];
static size_t ind_count = 0;

static GLuint atlas_tex;

static const char* vs_src =
    "#version 410 core\n"
    "layout(location = 0) in vec2 inPos;\n"
    "layout(location = 1) in vec2 inTexCoord;\n"
    "layout(location = 2) in vec4 inColor;\n"
    "uniform mat4 uMvp;"
    "out vec2 TexCoord;\n"
    "out vec4 Color;\n"
    "void main()\n"
    "{\n"
    "    gl_Position = uMvp * vec4(inPos, 0.0, 1.0);\n"
    "    TexCoord = inTexCoord;\n"
    "    Color = inColor;\n"
    "}\n";

static const char* fs_src =
    "#version 410 core\n"
    "in vec2 TexCoord;\n"
    "in vec4 Color;\n"
    "uniform sampler2D uTex;"
    "out vec4 FragColor;\n"
    "void main()\n"
    "{\n"
    "    vec4 t = texture(uTex, TexCoord);\n"
    "    FragColor = t * Color;\n"
    "}\n";

static void _add_indices(u32 base, u32 *indices, int count)
{
    for (int i = 0; i < count; i++)
    {
        ind_buf[ind_count++] = base + indices[i];
    }
}

static void _get_atlas_q_verts(v2i cell_p, v2 out_verts[4])
{
    f32 min_x = cell_p.x * ATLAS_CELL_DIM;
    f32 max_x = min_x + ATLAS_CELL_DIM;
    min_x += ATLAS_CELL_PAD;
    max_x -= ATLAS_CELL_PAD;

    f32 max_y = ATLAS_DIM - cell_p.y * ATLAS_CELL_DIM;
    f32 min_y = max_y - ATLAS_CELL_DIM;
    max_y -= ATLAS_CELL_PAD;
    min_y += ATLAS_CELL_PAD;

    // texel offset, to sample the middle of texels
    min_x += 0.5f;
    max_x += 0.5f;
    min_y += 0.5f;
    max_y += 0.5f;

    // Normalized range
    min_x /= ATLAS_DIM;
    max_x /= ATLAS_DIM;
    min_y /= ATLAS_DIM;
    max_y /= ATLAS_DIM;

    out_verts[0].x = min_x; out_verts[0].y = min_y;
    out_verts[1].x = max_x; out_verts[1].y = min_y;
    out_verts[2].x = max_x; out_verts[2].y = max_y;
    out_verts[3].x = min_x; out_verts[3].y = max_y;

    // for (int i = 0; i < 4; i++)
    //     trace("atlas_vert[%d]: %f, %f", i, out_verts[i].x, out_verts[i].y);
}

static void _get_verts_for_p_r(v2 p, f32 r, v2 out_verts[4])
{
    out_verts[0].x = p.x - r; out_verts[0].y = p.y - r;
    out_verts[1].x = p.x + r; out_verts[1].y = p.y - r;
    out_verts[2].x = p.x + r; out_verts[2].y = p.y + r;
    out_verts[3].x = p.x - r; out_verts[3].y = p.y + r;
}

static void _load_atlas_texture()
{
    stbi_set_flip_vertically_on_load(true);
    int w, h, ch;
    unsigned char *tex_data = stbi_load(ATLAS_PATH, &w, &h, &ch, 4);
    assert(ch == 4);

    // GLenum internal_format, format;
    // if (ch == 4) { internal_format = GL_RGBA8; format = GL_RGBA; }
    // else if (ch == 3) { internal_format = GL_RGB8; format = GL_RGB; }
    // else if (ch == 2) { internal_format = GL_RG8; format = GL_RG; }
    // else if (ch == 1) { internal_format = GL_R8; format = GL_RED; }
    // else warning("Invalid texture channel number");

    glGenTextures(1, &atlas_tex);
    glBindTexture(GL_TEXTURE_2D, atlas_tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, tex_data);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glBindTexture(GL_TEXTURE_2D, 0);

    stbi_image_free(tex_data);
}

void ui_renderer_init()
{
    shader_program = glg__create_shader_program(vs_src, fs_src);

    glUseProgram(shader_program);
    shader_loc_uMvp = glGetUniformLocation(shader_program, "uMvp");
    shader_loc_uTex = glGetUniformLocation(shader_program, "uTex");
    glUniformMatrix4fv(shader_loc_uMvp, 1, GL_FALSE, m4_identity().d);
    glUniform1i(shader_loc_uTex, 0);

    glUseProgram(0);

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

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, VERT_SIZE, (void*)offsetof(struct Vert, tex_coord));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, VERT_SIZE, (void*)offsetof(struct Vert, color));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    _load_atlas_texture();
    glActiveTexture(ATLAS_TEXTURE_UNIT);
    glBindTexture(GL_TEXTURE_2D, atlas_tex);
}

void ui_renderer_submit_quad(v2 a, v2 b, v2 c, v2 d, v4 color)
{
    if (quad_count >= MAX_QUADS)
    {
        warning("Max quad_count reached");
        return;
    }

    int ind_base = quad_count * 4;

    v2 atlas_q_verts[4];
    _get_atlas_q_verts(V2I(0, 0), atlas_q_verts);

    quad_buf[quad_count++] = (struct Quad){
        (struct Vert){a, atlas_q_verts[0], color},
        (struct Vert){b, atlas_q_verts[1], color},
        (struct Vert){c, atlas_q_verts[2], color},
        (struct Vert){d, atlas_q_verts[3], color},
    };

    _add_indices(ind_base, (u32[]){0, 1, 2, 0, 2, 3}, 6);
}

void ui_renderer_submit_circle(v2 p, f32 r, v4 color)
{
    if (quad_count >= MAX_QUADS)
    {
        warning("Max quad_count reached");
        return;
    }

    int ind_base = quad_count * 4;

    v2 atlas_q_verts[4];
    _get_atlas_q_verts(V2I(2, 0), atlas_q_verts);

    v2 screen_quad_verts[4];
    _get_verts_for_p_r(p, r, screen_quad_verts);

    quad_buf[quad_count++] = (struct Quad){
        (struct Vert){screen_quad_verts[0], atlas_q_verts[0], color},
        (struct Vert){screen_quad_verts[1], atlas_q_verts[1], color},
        (struct Vert){screen_quad_verts[2], atlas_q_verts[2], color},
        (struct Vert){screen_quad_verts[3], atlas_q_verts[3], color},
    };

    _add_indices(ind_base, (u32[]){0, 1, 2, 0, 2, 3}, 6);
}

void ui_renderer_draw(v2 window_size)
{
    glUseProgram(shader_program);
    m4 proj = m4_proj_ortho(0.0f, window_size.x, window_size.y, 0.0f, -1.0f, 1.0f);

    glUniformMatrix4fv(shader_loc_uMvp, 1, GL_FALSE, proj.d);

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
