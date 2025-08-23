#include "glfw_glue.h"

#include <OpenGL/gl3.h>
#include <GLFW/glfw3.h>

#include "util.h"
#include "types.h"

#define AVG_TIME_MEASUREMENT_PERIOD 0.1f

static GLFWwindow *g_Window;
static f32 g_PrevInstMsrmTime;
static f32 g_PrevAvgMsrmTime;
static int g_AvgMsrmFrameCount;
static f32 g_DeltaTime;
static f32 g_FPSAverage;

GLFWwindow *glfwg__init(int window_w, int window_h)
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    g_Window = glfwCreateWindow(window_w, window_h, "edi2tor", NULL, NULL);
    glfwMakeContextCurrent(g_Window);

    trace("OpenGL Vendor: %s", glGetString(GL_VENDOR));
    trace("OpenGL Renderer: %s", glGetString(GL_RENDERER));
    trace("OpenGL Version: %s", glGetString(GL_VERSION));
    trace("GLSL Version: %s", glGetString(GL_SHADING_LANGUAGE_VERSION));

    f32 current_time = (f32)glfwGetTime();
    g_PrevInstMsrmTime = current_time;
    g_PrevAvgMsrmTime = current_time;

    return g_Window;
}

v2i glfwg__get_fb_size()
{
    v2i result;
    glfwGetFramebufferSize(g_Window, &result.x, &result.y);
    return result;
}

v2i glfwg__get_window_size()
{
    v2i result;
    glfwGetWindowSize(g_Window, &result.x, &result.y);
    return result;
}

v2 glfwg__get_window_size_f()
{
    int w, h;
    glfwGetWindowSize(g_Window, &w, &h);
    return V2(w, h);
}

void glfwg__perform_timing_calculations()
{
    f32 current_frame_time = (f32)glfwGetTime();
    g_DeltaTime = current_frame_time - g_PrevInstMsrmTime;
    g_PrevInstMsrmTime = current_frame_time;

    f32 avg_measurement_delta = current_frame_time - g_PrevAvgMsrmTime;
    if (avg_measurement_delta > AVG_TIME_MEASUREMENT_PERIOD)
    {
        g_FPSAverage = (f32) g_AvgMsrmFrameCount / avg_measurement_delta;
        g_AvgMsrmFrameCount = 0;
        g_PrevAvgMsrmTime = current_frame_time;
    }
    g_AvgMsrmFrameCount++;
}

f32 glfwg__get_delta_time()
{
    return g_DeltaTime;
}

f32 glfwg__get_avg_fps()
{
    return g_FPSAverage;
}

bool glfwg__is_key_pressed(int key)
{
    return glfwGetKey(g_Window, key) == GLFW_PRESS;
}
