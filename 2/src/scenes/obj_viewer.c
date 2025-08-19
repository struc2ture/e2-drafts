#include <stdint.h>

#include <OpenGL/gl3.h>
#include <GLFW/glfw3.h>

#include "../hub/hub.h"
#include "../lib/common.h"
#include "../lib/gl_glue.h"
#include "../lib/util.h"

struct Triangle_State
{
    GLuint prog;
    struct Vert_Buf *vb;
    float w, h;
    float rot_angle;
    bool mouse_pressed;
};

void on_init(struct Triangle_State *s, struct Hub_Context *hub_context)
{
    s->w = hub_context->window_px_w;
    s->h = hub_context->window_px_h;
    s->prog = gl_create_basic_shader();
    s->vb = vb_make();
}

void on_reload(struct Triangle_State *s)
{
}

void on_frame(struct Triangle_State *s, const struct Hub_Timing *t)
{
    glViewport(0, 0, s->w, s->h);
    glUseProgram(s->prog);

    glEnable(GL_DEPTH_TEST);

    vb_clear(s->vb);

    int index_base = vb_next_vert_index(s->vb);

    Vec_3 verts[] =
    {
        {-0.5f, -0.5f, -0.5f},
        { 0.5f, -0.5f, -0.5f},
        { 0.5f, -0.5f,  0.5f},
        {-0.5f, -0.5f,  0.5f},
        {-0.5f,  0.5f, -0.5f},
        { 0.5f,  0.5f, -0.5f},
        { 0.5f,  0.5f,  0.5f},
        {-0.5f,  0.5f,  0.5f},
    };

    struct Col4f colors[] =
    {
        {0.5f, 0.0f, 0.0f, 1.0f},
        {0.0f, 0.5f, 0.0f, 1.0f},
        {0.0f, 0.0f, 0.5f, 1.0f},
        {0.5f, 0.0f, 0.5f, 1.0f},
        {1.0f, 0.0f, 0.0f, 1.0f},
        {0.0f, 1.0f, 0.0f, 1.0f},
        {0.0f, 0.0f, 1.0f, 1.0f},
        {1.0f, 0.0f, 1.0f, 1.0f},
    };

    Mat_4 proj = mat4_proj_perspective(PI32 / 3, s->w / s->h, 0.1f, 100.0f);
    Mat_4 view = mat4_look_at(
        (Vec_3){0.0f, 0.0f, 5.0f},
        (Vec_3){0},
        (Vec_3){0, 1.0, 0.0f}
    );

    Mat_4 pitch_rot = mat4_rotate_axis(1, 0, 0, deg_to_rad(25));
    Vec_3 axis = vec3_normalize((Vec_3){0.0f, 1.0f, 0.0f});
    Mat_4 yaw_rot = mat4_rotate_axis(axis.x, axis.y, axis.z, deg_to_rad(s->rot_angle));
    Mat_4 model = mat4_mul(pitch_rot, yaw_rot);

    Mat_4 mvp = mat4_mul(proj, mat4_mul(view, model));

    glUniformMatrix4fv(glGetUniformLocation(s->prog, "u_mvp"), 1, GL_FALSE, mvp.m);

    for (int i = 0; i < (int)array_size(verts); i++)
    {
        Vec_3 v = verts[i];
        // v = mat4_mul_vec3(rot, v);
        vb_add_vert(s->vb, (struct Vert){v.x, v.y, v.z, 0.0f, 0.0f, colors[i]});
    }

    int indices[] =
    {
        0, 1, 2,
        2, 3, 0,
        4, 5, 6,
        6, 7, 4,
        3, 2, 6,
        6, 7, 3,
        0, 1, 5,
        5, 4, 0,
        0, 3, 7,
        7, 4, 0,
        1, 2, 6,
        6, 5, 1
    };

    vb_add_indices(s->vb, index_base, indices, array_size(indices));

    vb_draw_call(s->vb);

    glDisable(GL_DEPTH_TEST);
}

void on_platform_event(struct Triangle_State *s, const struct Hub_Event *e)
{
    switch (e->kind)
    {
        case HUB_EVENT_MOUSE_BUTTON:
        {
            if (e->mouse_button.button == GLFW_MOUSE_BUTTON_LEFT)
            {
                if (e->mouse_button.action == GLFW_PRESS)
                {
                    s->mouse_pressed = true;
                }
                else if (e->mouse_button.action == GLFW_RELEASE)
                {
                    s->mouse_pressed = false;
                }
            }
        } break;

        case HUB_EVENT_MOUSE_MOTION:
        {
            if (s->mouse_pressed)
            {
                s->rot_angle += e->mouse_motion.delta.x;
            }
        } break;

        default: break;
    }
}

void on_destroy(struct Triangle_State *s)
{
    if (s->prog) glDeleteProgram(s->prog);
    if (s->vb) vb_free(s->vb);
}
