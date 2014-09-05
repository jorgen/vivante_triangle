#ifndef TRIANGLERENDERER_H
#define TRIANGLERENDERER_H

#include <EGL/egl.h>
#include <GLES2/gl2.h>


struct Size
{
    Size()
        : width(0)
        , height(0)
    { }
    Size(int width, int height)
        : width(width)
        , height(height)
    { }

    int width;
    int height;
};

class TriangleRenderer
{
public:
    TriangleRenderer(int display, TriangleRenderer *shareWith);

    void initializeProgram();
    void bindProgram();
    void releaseProgram();

    void makeCurrent();

    void renderFrame();

    void swapBuffers();

    Size size() const;
    void rotate(float angle);
private:

    Size m_size;

    EGLDisplay m_display;
    EGLContext m_context;
    EGLSurface m_surface;

    GLuint m_program;
    GLfloat m_rotation_matrix[4][4];
    int m_rotation_pos;
    int m_vertex_position_pos;
    int m_color_pos;
};

#endif // TRIANGLERENDERER_H
