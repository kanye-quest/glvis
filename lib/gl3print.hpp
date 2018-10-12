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

#ifndef __GLVIS_GL3PRINT__
#define __GLVIS_GL3PRINT__

#include "glstate.hpp"
#include "aux_gl3.hpp"

#include <iostream>

namespace gl3
{

/**
 * Helper functions for using GL2PS with OpenGL 3+ transform feedback.
 */
struct FeedbackVertex
{
    float pos[4];
    float color[4];
    float clipCoord;
};

void processTriangleTransformFeedback(FeedbackVertex * buf, size_t numVerts);
void processLineTransformFeedback(FeedbackVertex * buf, size_t numVerts);

class GL2PSFeedbackHook : public IDrawHook
{
private:
    GLuint _feedback_buf;
public:
    GL2PSFeedbackHook() {
        glGenBuffers(1, &_feedback_buf);
    }

    ~GL2PSFeedbackHook() {
        if (_feedback_buf != 0) {
            glDeleteBuffers(1, &_feedback_buf);
        }
    }

    GL2PSFeedbackHook(GL2PSFeedbackHook&& other)
        : _feedback_buf(other._feedback_buf) {
        other._feedback_buf = 0;
    }

    GL2PSFeedbackHook& operator = (GL2PSFeedbackHook&& other) {
        if (this != &other) {
            _feedback_buf = other._feedback_buf;
            other._feedback_buf = 0;
        }
        return *this;
    }

    void preDraw(GLenum shape, const IVertexBuffer * d) {
        // Allocate buffer and setup feedback
        int buf_size = d->count() * sizeof(FeedbackVertex);
        glBindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, _feedback_buf);
        glBufferData(GL_TRANSFORM_FEEDBACK_BUFFER,
                     buf_size, nullptr, GL_STATIC_READ);
        glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, _feedback_buf);

        // Draw objects while capturing vertices
        glEnable(GL_RASTERIZER_DISCARD);
        glBeginTransformFeedback(shape);
    }

    void postDraw(GLenum shape, const IVertexBuffer * d) {
        int buf_size = d->count() * sizeof(FeedbackVertex);
        glEndTransformFeedback();
        glDisable(GL_RASTERIZER_DISCARD);
        // Read buffer
        glMapBufferRange(GL_TRANSFORM_FEEDBACK_BUFFER, 0, buf_size,
                         GL_MAP_READ_BIT);
        FeedbackVertex * buf_mapped = nullptr;
        glGetBufferPointerv(GL_TRANSFORM_FEEDBACK_BUFFER,
                            GL_BUFFER_MAP_POINTER,
                            (GLvoid**)(&buf_mapped));
        if (shape == GL_TRIANGLES) {
            processTriangleTransformFeedback(buf_mapped, d->count());
        } else if (shape == GL_LINES) {
            processLineTransformFeedback(buf_mapped, d->count());
        } else { //shape == GL_POINTS/other?
            std::cerr << "Warning: Unhandled primitive type during transform feedback parsing.";
        }
        // Cleanup
        glUnmapBuffer(GL_TRANSFORM_FEEDBACK_BUFFER);
    }

    void preDraw(const TextBuffer& t) {
        glEnable(GL_RASTERIZER_DISCARD);
    }

    void postDraw(const TextBuffer& t) {
        glDisable(GL_RASTERIZER_DISCARD);
    }

};

}

#endif /* __GLVIS_GLPRINT__ */
