#include "glstate.hpp"
#include <string>
#include <iostream>

using std::cerr;
using std::endl;

#ifdef __EMSCRIPTEN__
const std::string _glsl_add = "precision mediump float;\n";
#else
const std::string _glsl_add = "#version 130\n";
#endif

const std::string vertex_shader_file = _glsl_add +
#include "shaders/clip_plane.vert"
+
#include "shaders/default.vert"
;

const std::string fragment_shader_file = _glsl_add +
#include "shaders/lighting.glsl"
+
R"(
void fragmentClipPlane() {
}
)"
+
#include "shaders/default.frag"
;

bool GlState::compileShaders() {
    GLuint vtx_shader = glCreateShader(GL_VERTEX_SHADER);
    GLuint frag_shader = glCreateShader(GL_FRAGMENT_SHADER);
    GLint success = 0;
    int vertex_shader_len = vertex_shader_file.length();
    const char * vert_str = vertex_shader_file.c_str();
    glShaderSource(vtx_shader, 1, &vert_str, &vertex_shader_len);
    glCompileShader(vtx_shader);
    glGetShaderiv(vtx_shader, GL_COMPILE_STATUS, &success);
    if (success == GL_FALSE) {
        cerr << "FATAL: Vertex shader compilation failed." << endl;
        int err_len;
        glGetShaderiv(vtx_shader, GL_INFO_LOG_LENGTH, &err_len);
        char * error_text = new char[err_len];
        glGetShaderInfoLog(vtx_shader, err_len, &err_len, error_text);
        cerr << error_text << endl;
        delete [] error_text;
        return false;
    }
    int frag_shader_len = fragment_shader_file.length();
    const char * frag_str = fragment_shader_file.c_str();
    glShaderSource(frag_shader, 1, &frag_str, &frag_shader_len);
    glCompileShader(frag_shader);
    glGetShaderiv(frag_shader, GL_COMPILE_STATUS, &success);
    if (success == GL_FALSE) {
        cerr << "FATAL: Fragment shader compilation failed." << endl;
        int err_len;
        glGetShaderiv(frag_shader, GL_INFO_LOG_LENGTH, &err_len);
        char * error_text = new char[err_len];
        glGetShaderInfoLog(frag_shader, err_len, &err_len, error_text);
        cerr << error_text << endl;
        delete [] error_text;
        return false;
    }
    program = glCreateProgram();
    //for OSX, attrib 0 must be bound to render an object
    glBindAttribLocation(program, 0, "vertex");
    glAttachShader(program, vtx_shader);
    glAttachShader(program, frag_shader);

    glLinkProgram(program);
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (success == GL_FALSE) {
        cerr << "FATAL: Shader linking failed." << endl;
        glDeleteProgram(program);
        program = 0;
    } else {
        glUseProgram(program);
    }

    glDetachShader(program, vtx_shader);
    glDetachShader(program, frag_shader);
    glDeleteShader(vtx_shader);
    glDeleteShader(frag_shader);
    return (success != GL_FALSE);
}

void GlState::initShaderState() {
    _attr_locs[ATTR_VERTEX] = glGetAttribLocation(program, "vertex");
    _attr_locs[ATTR_TEXT_VERTEX] = glGetAttribLocation(program, "textVertex");
    _attr_locs[ATTR_COLOR] = glGetAttribLocation(program, "color");
    _attr_locs[ATTR_NORMAL] = glGetAttribLocation(program, "normal");
    _attr_locs[ATTR_TEXCOORD0] = glGetAttribLocation(program, "texCoord0");
    _attr_locs[ATTR_TEXCOORD1] = glGetAttribLocation(program, "texCoord1");

    locUseClipPlane = glGetUniformLocation(program, "useClipPlane");
    locClipPlane = glGetUniformLocation(program, "clipPlane");
    
    locModelView = glGetUniformLocation(program, "modelViewMatrix");
    locProject = glGetUniformLocation(program, "projectionMatrix");
    locProjectText = glGetUniformLocation(program, "textProjMatrix");
    locNormal = glGetUniformLocation(program, "normalMatrix");

    locSpec = glGetUniformLocation(program, "material.specular");
    locShin = glGetUniformLocation(program, "material.shininess");

    //Texture unit 0: color palettes
    //Texture unit 1: font atlas
    GLuint locColorTex = glGetUniformLocation(program, "colorTex");
    GLuint locFontTex = glGetUniformLocation(program, "fontTex");
    glUniform1i(locColorTex, 0);
    glUniform1i(locFontTex, 1);
    GLuint locContainsText = glGetUniformLocation(program, "containsText");
    glUniform1i(locContainsText, GL_FALSE);
    GLuint locUseColorTex = glGetUniformLocation(program, "useColorTex");
    glUniform1i(locUseColorTex, GL_FALSE);
    _shaderMode = RENDER_COLOR;
    modelView.identity();
    projection.identity();
    loadMatrixUniforms();

    glGenVertexArrays(1, &global_vao);
    glBindVertexArray(global_vao);
}
