#include "text_renderer.h"

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include <OpenGL/gl3.h>

#include "../common/gl_glue.h"
#include "../common/lin_math.h"
#include "../common/types.h"
#include "../common/util.h"

// #include <stb_image.h>
#include <stb_truetype.h>

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

#define FONT_PATH "res/font_dm_mono_italic.ttf"
#define FONT_SIZE 32.0f

#define FONT_TEXTURE_UNIT 1
#define FONT_TEXTURE_UNIT_ENUM (GL_TEXTURE0 + FONT_TEXTURE_UNIT)

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

static GLuint font_tex;
static f32 i_dpi_scale;

static int atlas_w;
static int atlas_h;
static int first_char;
static int char_count;
static f32 ascent;
static f32 descent;
static f32 line_gap;
static stbtt_bakedchar *char_data;

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
    "    float t = texture(uTex, TexCoord).r;\n"
    "    FragColor = t * Color;\n"
    "}\n";

static void _add_indices(u32 base, u32 *indices, int count)
{
    for (int i = 0; i < count; i++)
    {
        ind_buf[ind_count++] = base + indices[i];
    }
}

static void _get_font_q_screen_verts_and_tex_coords(stbtt_aligned_quad q, v2 out_screen_verts[4], v2 out_tex_coords[4])
{
    // f32 min_x = cell_p.x * ATLAS_CELL_DIM;
    // f32 max_x = min_x + ATLAS_CELL_DIM;
    // min_x += ATLAS_CELL_PAD;
    // max_x -= ATLAS_CELL_PAD;

    // f32 max_y = ATLAS_DIM - cell_p.y * ATLAS_CELL_DIM;
    // f32 min_y = max_y - ATLAS_CELL_DIM;
    // max_y -= ATLAS_CELL_PAD;
    // min_y += ATLAS_CELL_PAD;

    // // texel offset, to sample the middle of texels
    // min_x += 0.5f;
    // max_x += 0.5f;
    // min_y += 0.5f;
    // max_y += 0.5f;

    // // Normalized range
    // min_x /= ATLAS_DIM;
    // max_x /= ATLAS_DIM;
    // min_y /= ATLAS_DIM;
    // max_y /= ATLAS_DIM;

    // out_verts[0].x = min_x; out_verts[0].y = min_y;
    // out_verts[1].x = max_x; out_verts[1].y = min_y;
    // out_verts[2].x = max_x; out_verts[2].y = max_y;
    // out_verts[3].x = min_x; out_verts[3].y = max_y;

    // for (int i = 0; i < 4; i++)
    //     trace("atlas_vert[%d]: %f, %f", i, out_verts[i].x, out_verts[i].y);

    // typedef struct
    // {
    // float x0,y0,s0,t0; // top-left
    // float x1,y1,s1,t1; // bottom-right
    // } stbtt_aligned_quad;

    out_screen_verts[0].x = q.x0; out_screen_verts[0].y = q.y0;
    out_screen_verts[1].x = q.x1; out_screen_verts[1].y = q.y0;
    out_screen_verts[2].x = q.x1; out_screen_verts[2].y = q.y1;
    out_screen_verts[3].x = q.x0; out_screen_verts[3].y = q.y1;

    out_tex_coords[0].x = q.s0; out_tex_coords[0].y = q.t0;
    out_tex_coords[1].x = q.s1; out_tex_coords[1].y = q.t0;
    out_tex_coords[2].x = q.s1; out_tex_coords[2].y = q.t1;
    out_tex_coords[3].x = q.s0; out_tex_coords[3].y = q.t1;
}

static void _get_verts_for_p_r(v2 p, f32 r, v2 out_verts[4])
{
    out_verts[0].x = p.x - r; out_verts[0].y = p.y - r;
    out_verts[1].x = p.x + r; out_verts[1].y = p.y - r;
    out_verts[2].x = p.x + r; out_verts[2].y = p.y + r;
    out_verts[3].x = p.x - r; out_verts[3].y = p.y + r;
}

static void _load_font()
{
    FILE *f = fopen(FONT_PATH, "rb");
    if (!f) fatal("Failed to open file for reading at %s", FONT_PATH);
    fseek(f, 0, SEEK_END);
    int file_size = ftell(f);
    fseek(f, 0, SEEK_SET);
    void *file_bytes = xmalloc(file_size);
    fread(file_bytes, 1, file_size, f);

    f32 dpi_scale = 1.0f;
    i_dpi_scale = 1.0f / dpi_scale;

    atlas_w = 512;
    atlas_h = 512;

    void *atlas_bitmap = xcalloc(atlas_w * atlas_h); // 1 byte per pixel

    first_char = 32;
    char_count = 96;
    char_data = xcalloc(char_count * sizeof(stbtt_bakedchar));
    stbtt_BakeFontBitmap(file_bytes, 0, FONT_SIZE * dpi_scale, atlas_bitmap, atlas_w, atlas_h, first_char, char_count, char_data);

    stbtt_GetScaledFontVMetrics(file_bytes, 0, FONT_SIZE * dpi_scale, &ascent, &descent, &line_gap);
    free(file_bytes);

    glActiveTexture(GL_TEXTURE31); // Do texture init on unit 31, to not mess up already setup textures
    glGenTextures(1, &font_tex);
    glBindTexture(GL_TEXTURE_2D, font_tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, atlas_w, atlas_h, 0, GL_RED, GL_UNSIGNED_BYTE, atlas_bitmap);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);

    free(atlas_bitmap);
}

void text_renderer_init()
{
    shader_program = glg__create_shader_program(vs_src, fs_src);

    glUseProgram(shader_program);
    shader_loc_uMvp = glGetUniformLocation(shader_program, "uMvp");
    shader_loc_uTex = glGetUniformLocation(shader_program, "uTex");
    glUniformMatrix4fv(shader_loc_uMvp, 1, GL_FALSE, m4_identity().d);
    glUniform1i(shader_loc_uTex, FONT_TEXTURE_UNIT);

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

    _load_font();
    glActiveTexture(FONT_TEXTURE_UNIT_ENUM);
    glBindTexture(GL_TEXTURE_2D, font_tex);
}

void text_renderer_submit_string(const char *str, v2 p, v4 color)
{
    if (quad_count >= MAX_QUADS)
    {
        warning("Max quad_count reached");
        return;
    }

    p.y += ascent * i_dpi_scale;
    while (*str)
    {
        char ch = *str++;
        if (ch >= 32)
        {
            stbtt_aligned_quad q;
            stbtt_GetBakedQuad(char_data, atlas_w, atlas_h, ch-32, &p.x, &p.y ,&q, 1, i_dpi_scale);
            // break();
            v2 screen_verts[4];
            v2 tex_coords[4];
            _get_font_q_screen_verts_and_tex_coords(q, screen_verts, tex_coords);

            int ind_base = quad_count * 4;

            quad_buf[quad_count++] = (struct Quad){
                (struct Vert){screen_verts[0], tex_coords[0], color},
                (struct Vert){screen_verts[1], tex_coords[1], color},
                (struct Vert){screen_verts[2], tex_coords[2], color},
                (struct Vert){screen_verts[3], tex_coords[3], color},
            };

            _add_indices(ind_base, (u32[]){0, 1, 2, 0, 2, 3}, 6);
        }
    }

}

void text_renderer_draw(v2 window_size)
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
