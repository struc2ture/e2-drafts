#include "glfw_glue.h"

#include <OpenGL/gl3.h>
#include <GLFW/glfw3.h>

#include "util.h"
#include "types.h"

static GLFWwindow *glfwg__g_Window;

static GLFWwindow *glfwg__init(int window_w, int window_h)
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwg__g_Window = glfwCreateWindow(window_w, window_h, "edi2tor", NULL, NULL);
    glfwMakeContextCurrent(glfwg__g_Window);

    trace("OpenGL Vendor: %s", glGetString(GL_VENDOR));
    trace("OpenGL Renderer: %s", glGetString(GL_RENDERER));
    trace("OpenGL Version: %s", glGetString(GL_VERSION));

    return glfwg__g_Window;
}

static v2i glfwg__get_fb_size()
{
    v2i result;
    glfwGetFramebufferSize(glfwg__g_Window, &result.x, &result.y);
    return result;
}
