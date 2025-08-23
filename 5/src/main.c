#include <stdio.h>

#include <OpenGL/gl3.h>
#include <GLFW/glfw3.h>

#include "common/colors.h"
#include "common/gl_glue.h"
#include "common/glfw_glue.h"
#include "common/lin_math.h"
#include "common/types.h"
#include "common/util.h"
#include "text_renderer/text_renderer.h"
#include "ui_renderer/ui_renderer.h"

void on_char(GLFWwindow *window, unsigned int codepoint)
{
}

void on_key(GLFWwindow *window, int key, int scancode, int action, int mods)
{
}

void on_mouse_cursor(GLFWwindow *window, double xpos, double ypos)
{
}

void on_mouse_button(GLFWwindow *window, int button, int action, int mods)
{
}

void on_scroll(GLFWwindow *window, double x_offset, double y_offset)
{
}

void on_framebuffer_size(GLFWwindow *window, int w, int h)
{
}

void on_window_size(GLFWwindow *window, int w, int h)
{
}

int main()
{
    int width =  1000;
    int height =  900;

    GLFWwindow *window = glfwg__init(width, height);

    glfwSetCharCallback(window, on_char);
    glfwSetKeyCallback(window, on_key);
    glfwSetCursorPosCallback(window, on_mouse_cursor);
    glfwSetMouseButtonCallback(window, on_mouse_button);
    glfwSetScrollCallback(window, on_scroll);
    glfwSetFramebufferSizeCallback(window, on_framebuffer_size);
    glfwSetWindowSizeCallback(window, on_window_size);

    glfwSwapInterval(1);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    ui_renderer_init();
    text_renderer_init();

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        glg__set_viewport_size(glfwg__get_fb_size());

        glg__clear(COLOR_ALIZARIN);

        v2 window_size = glfwg__get_window_size_f();
        v2 screen_middle = V2(window_size.x * 0.5f, window_size.y * 0.5f);

        f32 quad1_r = 200.0f;
        f32 quad2_r = 150.0f;
        f32 quad3_r = 100.0f;

        ui_renderer_submit_quad(
            v2_add(screen_middle, V2(-quad1_r,  quad1_r)),
            v2_add(screen_middle, V2( quad1_r,  quad1_r)),
            v2_add(screen_middle, V2( quad1_r, -quad1_r)), 
            v2_add(screen_middle, V2(-quad1_r, -quad1_r)),
            COLOR_BLACK
        );

        ui_renderer_submit_quad(
            v2_add(screen_middle, V2(-quad2_r,  quad2_r)),
            v2_add(screen_middle, V2( quad2_r,  quad2_r)),
            v2_add(screen_middle, V2( quad2_r, -quad2_r)), 
            v2_add(screen_middle, V2(-quad2_r, -quad2_r)),
            COLOR_WHITE
        );

        ui_renderer_submit_quad(
            v2_add(screen_middle, V2(-quad3_r,  quad3_r)),
            v2_add(screen_middle, V2( quad3_r,  quad3_r)),
            v2_add(screen_middle, V2( quad3_r, -quad3_r)), 
            v2_add(screen_middle, V2(-quad3_r, -quad3_r)),
            COLOR_ALIZARIN
        );

        ui_renderer_submit_circle(screen_middle, 16.0f, COLOR_BLACK);

        ui_renderer_draw(window_size);

        text_renderer_submit_string(
            V2(200.0f, 600.0f),
            V2(600.0f, 600.0f),
            V2(600.0f, 200.0f),
            V2(200.0f, 200.0f),
            COLOR_WHITE
        );

        text_renderer_draw(window_size);

        glfwSwapBuffers(window);
    }

    return 0;
}
