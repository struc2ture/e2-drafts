#include <stdio.h>

#include <OpenGL/gl3.h>
#include <GLFW/glfw3.h>

#include "common/colors.h"
#include "common/gl_glue.h"
#include "common/glfw_glue.h"
#include "common/types.h"
#include "common/util.h"
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

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        glg__set_viewport_size(glfwg__get_fb_size());

        glg__clear(COLOR_ALIZARIN);

        ui_renderer_submit_quad(V2(-0.6f, -0.6f), V2(0.6f, -0.6f), V2(0.6f, 0.6f), V2(-0.6f, 0.6f), COLOR_BLACK);
        ui_renderer_submit_quad(V2(-0.5f, -0.5f), V2(0.5f, -0.5f), V2(0.5f, 0.5f), V2(-0.5f, 0.5f), COLOR_ALIZARIN);
        ui_renderer_submit_quad(V2(-0.25f, -0.25f), V2(0.25f, -0.25f), V2(0.25f, 0.25f), V2(-0.25f, 0.25f), COLOR_WHITE);
        ui_renderer_submit_circle(V2(0.0f, 0.0f), 0.01f, COLOR_ALIZARIN);
        ui_renderer_draw();

        glfwSwapBuffers(window);
    }

    return 0;
}
