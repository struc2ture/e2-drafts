#include "glfw_glue.h"

#include <OpenGL/gl3.h>
#include <GLFW/glfw3.h>

#include "util.h"
#include "types.h"

#define AVG_TIME_MEASUREMENT_PERIOD 0.1f

static GLFWwindow *glfwg__g_Window;
static f32 glfwg__g_PrevInstMsrmTime;
static f32 glfwg__g_PrevAvgMsrmTime;
static int glfwg__g_AvgMsrmFrameCount;
static f32 glfwg__g_DeltaTime;
static f32 glfwg__g_FPSAverage;

GLFWwindow *glfwg__init(int window_w, int window_h)
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
    trace("GLSL Version: %s", glGetString(GL_SHADING_LANGUAGE_VERSION));

    f32 current_time = (f32)glfwGetTime();
    glfwg__g_PrevInstMsrmTime = current_time;
    glfwg__g_PrevAvgMsrmTime = current_time;

    return glfwg__g_Window;
}

v2i glfwg__get_fb_size()
{
    v2i result;
    glfwGetFramebufferSize(glfwg__g_Window, &result.x, &result.y);
    return result;
}

v2i glfwg__get_window_size()
{
    v2i result;
    glfwGetWindowSize(glfwg__g_Window, &result.x, &result.y);
    return result;
}

v2 glfwg__get_window_size_f()
{
    int w, h;
    glfwGetWindowSize(glfwg__g_Window, &w, &h);
    return V2(w, h);
}

void glfwg__perform_timing_calculations()
{
    f32 current_frame_time = (f32)glfwGetTime();
    glfwg__g_DeltaTime = current_frame_time - glfwg__g_PrevInstMsrmTime;
    glfwg__g_PrevInstMsrmTime = current_frame_time;

    f32 avg_measurement_delta = current_frame_time - glfwg__g_PrevAvgMsrmTime;
    if (avg_measurement_delta > AVG_TIME_MEASUREMENT_PERIOD)
    {
        glfwg__g_FPSAverage = (f32) glfwg__g_AvgMsrmFrameCount / avg_measurement_delta;
        glfwg__g_AvgMsrmFrameCount = 0;
        glfwg__g_PrevAvgMsrmTime = current_frame_time;
    }
    glfwg__g_AvgMsrmFrameCount++;
}

f32 glfwg__get_delta_time()
{
    return glfwg__g_DeltaTime;
}

f32 glfwg__get_avg_fps()
{
    return glfwg__g_FPSAverage;
}
