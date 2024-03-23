#ifndef _OPENGL_HH_
#define _OPENGL_HH_

#include <glad/glad.h>

#include "hk.hh"

using namespace hk;

#ifndef GLCHECK
    #define GLCHECK(code) do {                                                                          \
        code;                                                                                           \
        const u32 glcheck_err = glGetError();                                                           \
        if (glcheck_err != GL_NO_ERROR) {                                                               \
            dbgerr("%s:%d: %s -> %s", __FILE__, __LINE__, #code, get_gl_error_name(glcheck_err));       \
        }                                                                                               \
    } while (0);
#endif

static inline const char* get_gl_error_name(GLenum error) {
    switch (error) {
    case GL_NO_ERROR:                       return "GL_NO_ERROR";
    case GL_INVALID_ENUM:                   return "GL_INVALID_ENUM";
    case GL_INVALID_VALUE:                  return "GL_INVALID_VALUE";
    case GL_INVALID_OPERATION:              return "GL_INVALID_OPERATION";
    case GL_INVALID_FRAMEBUFFER_OPERATION:  return "GL_INVALID_FRAMEBUFFER_OPERATION";
    case GL_OUT_OF_MEMORY:                  return "GL_OUT_OF_MEMORY";
    default:                                return "(unknown error)";
    };
};

static inline GLuint compile_gl_program(const char* vs_path, const char* fs_path) {
    GLuint prog = glCreateProgram(); GLCHECK();

    struct {
        GLenum type;
        const char* path;
    } shaders[] = {
        { GL_VERTEX_SHADER,     vs_path },
        { GL_FRAGMENT_SHADER,   fs_path },
    };

    for (usize i = 0; i < arrlen(shaders); ++i) {
        std::string source = load_text_file(shaders[i].path);
        if (source.empty()) {
            dbgerr("Failed to load %s", shaders[i].path);
        }
        const char* source_c = source.c_str();
        GLuint shader = glCreateShader(shaders[i].type); GLCHECK();
        GLCHECK(glShaderSource(shader, 1, (const GLchar**)&source_c, NULL));
        GLCHECK(glCompileShader(shader));
        GLint status = 0;
        GLCHECK(glGetShaderiv(shader, GL_COMPILE_STATUS, &status));
        if (status != GL_TRUE) {
            char log[512];
            GLCHECK(glGetShaderInfoLog(shader, sizeof(log), NULL, log));
            dbgerr("Error while compiling shader %s: %s", shaders[i].path, log);
        }
        GLCHECK(glAttachShader(prog, shader));
    }

    GLCHECK(glLinkProgram(prog));
    GLint link_status = 0;
    GLCHECK(glGetProgramiv(prog, GL_LINK_STATUS, &link_status));
    if (link_status != GL_TRUE) {
        char log[512];
        GLCHECK(glGetProgramInfoLog(prog, sizeof(prog), NULL, log));
        dbgerr("Error while linking program [%s, %s]: %s", vs_path, fs_path, log);
    }

    dbglog("Compiled shader [%s, %s]", vs_path, fs_path);

    return prog;
}

#endif // _OPENGL_HH_