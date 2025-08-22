#include "gl_glue.h"

#include <OpenGL/gl3.h>

#include "types.h"
#include "util.h"

void glg__set_viewport_size(v2i size)
{
    glViewport(0, 0, size.x, size.y);
}

void glg__clear(v4 c)
{
    glClearColor(c.r, c.g, c.b, c.a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

bool glg__check_compile_success(GLuint shader, const char *src)
{
    GLint success = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        char log[512];
        glGetShaderInfoLog(shader, sizeof(log), NULL, log);
        warning("Shader compile error:\n%s\n\nSource:", log);
        warning("%s\n", src);
    }
    return (bool)success;
}

bool glg__check_link_success(GLuint prog)
{
    GLint success = 0;
    glGetProgramiv(prog, GL_LINK_STATUS, &success);
    if (!success)
    {
        char log[512];
        glGetProgramInfoLog(prog, sizeof(log), NULL, log);
        warning("Program link error:\n%s", log);
    }
    return (bool)success;
}

GLuint glg__create_shader_program(const char *vs_src, const char *fs_src)
{
    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, 1, &vs_src, 0);
    glCompileShader(vs);
    glg__check_compile_success(vs, vs_src);

    GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs, 1, &fs_src, 0);
    glCompileShader(fs);
    glg__check_compile_success(fs, fs_src);

    GLuint prog = glCreateProgram();
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glLinkProgram(prog);
    glg__check_link_success(prog);

    glDeleteShader(vs);
    glDeleteShader(fs);

    return prog;
}
