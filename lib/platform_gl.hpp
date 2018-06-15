// Copyright (c) 2010, Lawrence Livermore National Security, LLC. Produced at
// the Lawrence Livermore National Laboratory. LLNL-CODE-443271. All Rights
// reserved. See file COPYRIGHT for details.
//
// This file is part of the GLVis visualization tool and library. For more
// information and source code availability see http://glvis.org.
//
// GLVis is free software; you can redistribute it and/or modify it under the
// terms of the GNU Lesser General Public License (as published by the Free
// Software Foundation) version 2.1 dated February 1999.

#ifndef PLATFORM_GL_HPP
#define PLATFORM_GL_HPP


#ifdef __EMSCRIPTEN__
#include <GL/glew.h>
#include <SDL.h>
#include <SDL_opengl.h>
//replace this with a custom matrix stack
inline void glGetDoublev(GLenum pname, GLdouble * params) {
    float data[16];
    glGetFloatv(pname, data);
    for (int i = 0; i < 16; i++) {
        params[i] = data[i];
    }
}
#else
#ifdef __APPLE__
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#else
#include <GL/gl.h>
#include <GL/glu.h>
#endif
#endif

#endif
