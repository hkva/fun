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
            dbgerr("%s:%d: %s -> %s", __FILE__, __LINE__, #code, GetGLErrorName(glcheck_err));           \
        }                                                                                               \
    } while (0);
#endif

static inline const char* GetGLErrorName(GLenum error) {
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

#endif // _OPENGL_HH_