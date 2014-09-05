#include "trianglerenderer.h"
#include <string>

#include <stdio.h>
#include <stdlib.h>

#include <cstring>
#include <cmath>

#include <GLES2/gl2.h>

static EGLint config_attribs[] = {
    EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
    EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
//    EGL_RED_SIZE, 8,
//    EGL_GREEN_SIZE, 8,
//    EGL_BLUE_SIZE, 8,
//    EGL_ALPHA_SIZE, 0,
    EGL_NONE
};

static EGLint context_attribs[] = {
    EGL_CONTEXT_CLIENT_VERSION, 2,
    EGL_NONE
};

static EGLint surface_attribs[] = {
    EGL_NONE
};

static const char vertex_shader_source[] =
    "attribute highp vec3 vertex_position;"
    "attribute highp vec3 color;"
    "uniform highp mat4 projection_view;"
    "uniform highp mat4 rotation;"
    "varying vec3 out_color;"
    "void main() {"
    "   out_color = color;"
    "   gl_Position = projection_view * rotation * vec4(vertex_position,1);"
    "}";

static const char fragment_shader_source[] =
    "varying vec3 out_color;"
    "void main() {"
    "   gl_FragColor = vec4(out_color,0);"
    "}";

static const GLfloat triangle_vertex_buffer_data[] = {
    -1.f, -1.f, 0.f,
     0.f,  1.f, 0.f,
     1.f, -1.f, 0.f,
};

static GLfloat projection_view_matrix[] = {
     0.570908, -0.690559,    -0.906345, -0.904534,
            0,  2.30186,     -0.302115, -0.301511,
     -1.71273, -0.230186,    -0.302115, -0.301511,
  1.07608e-07, -3.59746e-08,  3.123060,  3.31662
};

static const GLfloat color_buffer_data[] = {
    1.f, 0.f, 0.f,
    0.f, 1.f, 0.f,
    0.f, 0.f, 1.f,
};

static void printOnGLError(int line)
{
    if (GLint error = glGetError())
        fprintf(stderr, "GLGetError x%x at line %d\n", error, line);
}

static void verifyCompiledShader(GLuint shader)
{
    GLint compile_success;
    glGetShaderiv(shader,GL_COMPILE_STATUS,&compile_success);
    if (!compile_success) {
        GLint length;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
        if (length > 0) {
            char buffer[length];
            GLint actual_length;
            glGetShaderInfoLog(shader, length,&actual_length,buffer);
            fprintf(stderr, "ShaderCompile log: %s\n", buffer);
        }
    }
}

static GLuint createShader(GLenum type, const std::string &source)
{
    GLuint shader = glCreateShader(type);
    const char *source_str = source.c_str();
    const int length = source.length();
    glShaderSource(shader,1,&source_str,&length);
    printOnGLError(__LINE__);
    glCompileShader(shader);
    verifyCompiledShader(shader);
    printOnGLError(__LINE__);
    return shader;
}

static void verifyLinkedProgram(GLuint program)
{
    GLint link_status;
    glGetProgramiv(program, GL_LINK_STATUS, &link_status);
    if (!link_status) {
        fprintf(stderr, "Link status %d\n", link_status);
        GLint length;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length);
        char buffer[length];
        GLint actualLength;
        glGetProgramInfoLog(program, length,&actualLength, buffer);
        fprintf(stderr, "Program log: %s\n", buffer);
    }
    glValidateProgram(program);
    printOnGLError(__LINE__);
}

static void printColorSizesOfConfig(EGLDisplay display, EGLConfig config)
{
    EGLint red,green,blue,alpha, config_id;
    if (!eglGetConfigAttrib(display, config, EGL_CONFIG_ID, &config_id)) {
        fprintf(stderr, "Failed to get EGL_CONFIG_ID, will not print color sizes\n");
        return;
    }
    if (!eglGetConfigAttrib(display, config, EGL_RED_SIZE, &red)) {
        fprintf(stderr, "Failed to get EGL_RED_SIZE, will not print color sizes\n");
        return;
    }
    if (!eglGetConfigAttrib(display, config, EGL_GREEN_SIZE, &green)) {
        fprintf(stderr, "Failed to get EGL_GREEN_SIZE, will not print color sizes\n");
        return;
    }
    if (!eglGetConfigAttrib(display, config, EGL_BLUE_SIZE, &blue)) {
        fprintf(stderr, "Failed to get EGL_BLUE_SIZE, will not print color sizes\n");
        return;
    }
    if (!eglGetConfigAttrib(display, config, EGL_ALPHA_SIZE, &alpha)) {
        fprintf(stderr, "Failed to get EGL_ALPHA_SIZE, will not print color sizes\n");
        return;
    }

    fprintf(stderr, "EGLconfig %d has following RGBA sizes %d : %d : %d : %d\n", config_id, red, green, blue, alpha);

}


TriangleRenderer::TriangleRenderer(int display, TriangleRenderer *shareWith)
    : m_program(0)
    , m_rotation_pos(-1)
    , m_vertex_position_pos(-1)
    , m_color_pos(-1)
{
    eglBindAPI(EGL_OPENGL_ES_API);
    EGLNativeDisplayType nativeDisplay = fbGetDisplayByIndex(display);
    fbGetDisplayGeometry(nativeDisplay,&m_size.width,&m_size.height);
    fprintf(stderr, "Width: %d Height: %d\n", m_size.width, m_size.height);
    m_display = eglGetDisplay(nativeDisplay);

    EGLint major, minor;
    EGLBoolean success;
    success = eglInitialize(m_display, &major, &minor);
    if (!success) {
        fprintf(stderr, "Failed to init display\n");
        abort();
    }
    EGLConfig config;
    EGLint number_of_configs;

    EGLContext shareContext = shareWith ? shareWith->m_context : EGL_NO_CONTEXT;
    EGLDisplay shareDisplay = shareWith ? shareWith->m_display : m_display;
    success = eglChooseConfig(shareDisplay, config_attribs, &config, 1, &number_of_configs);
    if (!success) {
        fprintf(stderr, "Failed to get config\n");
        abort();
    }
    printColorSizesOfConfig(shareDisplay, config);


    m_context = eglCreateContext(shareDisplay, config, shareContext, context_attribs);
    if (m_context == EGL_NO_CONTEXT){
        fprintf(stderr, "Failed to create context\n");
        abort();
    }

    EGLNativeWindowType nativeWindow = fbCreateWindow(nativeDisplay,0,0,m_size.width,m_size.height);
    m_surface = eglCreateWindowSurface(m_display,config,nativeWindow, surface_attribs);

    memset(&m_rotation_matrix[0][0],0,sizeof(m_rotation_matrix));
    m_rotation_matrix[0][0] = 1.f;
    m_rotation_matrix[1][1] = 1.f;
    m_rotation_matrix[2][2] = 1.f;
    m_rotation_matrix[3][3] = 1.f;
}

void TriangleRenderer::initializeProgram()
{
    m_program = glCreateProgram();
    printOnGLError(__LINE__);

    GLuint vertex_shader = createShader(GL_VERTEX_SHADER, vertex_shader_source);
    GLuint fragment_shader = createShader(GL_FRAGMENT_SHADER, fragment_shader_source);

    glAttachShader(m_program, vertex_shader);
    glAttachShader(m_program, fragment_shader);
    printOnGLError(__LINE__);
    glLinkProgram(m_program);
    printOnGLError(__LINE__);
    verifyLinkedProgram(m_program);

    glUseProgram(m_program);
    printOnGLError(__LINE__);

    int projection_pos = glGetUniformLocation(m_program, "projection_view");
    glUniformMatrix4fv(projection_pos,1,GL_FALSE, projection_view_matrix);
    printOnGLError(__LINE__);

    m_vertex_position_pos = glGetAttribLocation(m_program, "vertex_position");
    GLuint vertex_buffer;
    glGenBuffers(1, &vertex_buffer);
    glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(triangle_vertex_buffer_data), triangle_vertex_buffer_data, GL_STATIC_DRAW);
    glVertexAttribPointer(m_vertex_position_pos, 3, GL_FLOAT, GL_FALSE, 0, 0);

    m_color_pos = glGetAttribLocation(m_program, "color");
    GLuint color_buffer;
    glGenBuffers(1, &color_buffer);
    glBindBuffer(GL_ARRAY_BUFFER, color_buffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(color_buffer_data), color_buffer_data, GL_STATIC_DRAW);
    glVertexAttribPointer(m_color_pos, 3, GL_FLOAT, GL_FALSE, 0, 0);

    m_rotation_pos = glGetUniformLocation(m_program, "rotation");

}

void TriangleRenderer::bindProgram()
{
    glUseProgram(m_program);
    glEnableVertexAttribArray(m_vertex_position_pos);
    glEnableVertexAttribArray(m_color_pos);
}

void TriangleRenderer::releaseProgram()
{
    glDisableVertexAttribArray(m_color_pos);
    glDisableVertexAttribArray(m_vertex_position_pos);
    glUseProgram(0);
}

void TriangleRenderer::makeCurrent()
{
    EGLBoolean success = eglMakeCurrent(m_display, m_surface, m_surface, m_context);
    if (!success)
        fprintf(stderr, "EGLCall %d at line: %d\n", success, __LINE__);
}

void TriangleRenderer::renderFrame()
{
    glViewport(0,0,m_size.width, m_size.height);
    glClearColor(0.0,0.3,0.2,0.0);
    glClear(GL_COLOR_BUFFER_BIT);
    glUniformMatrix4fv(m_rotation_pos, 1, GL_FALSE, &m_rotation_matrix[0][0]);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    printOnGLError(__LINE__);
}

void TriangleRenderer::swapBuffers()
{
    EGLBoolean success = eglSwapBuffers(m_display, m_surface);
    if (!success)
        fprintf(stderr, "EGLCall %d at line: %d\n", success, __LINE__);
}

void TriangleRenderer::rotate(float angle)
{
    float a = angle * M_PI / 180.0f;
    float c = cos(a);
    float s = sin(a);

    float tmp;
    m_rotation_matrix[2][0] = (tmp = m_rotation_matrix[2][0]) * c + m_rotation_matrix[0][0] * s;
    m_rotation_matrix[0][0] = m_rotation_matrix[0][0] * c - tmp * s;
    m_rotation_matrix[2][1] = (tmp = m_rotation_matrix[2][1]) * c + m_rotation_matrix[0][1] * s;
    m_rotation_matrix[0][1] = m_rotation_matrix[0][1] * c - tmp * s;
    m_rotation_matrix[2][2] = (tmp = m_rotation_matrix[2][2]) * c + m_rotation_matrix[0][2] * s;
    m_rotation_matrix[0][2] = m_rotation_matrix[0][2] * c - tmp * s;
    m_rotation_matrix[2][3] = (tmp = m_rotation_matrix[2][3]) * c + m_rotation_matrix[0][3] * s;
    m_rotation_matrix[0][3] = m_rotation_matrix[0][3] * c - tmp * s;
}
