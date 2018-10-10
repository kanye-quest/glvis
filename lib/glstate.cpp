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

enum ShaderFile {
    VS_CLIP_PLANE = 0,
    FS_CLIP_PLANE,
    VS_LIGHTING,
    FS_LIGHTING,
    VS_DEFAULT,
    FS_DEFAULT,
    VS_PRINTING,
    FS_PRINTING,
    NUM_SHADERS
};

const std::string shader_files[] = {
#include "shaders/clip_plane.vert"
,
#include "shaders/clip_plane.frag"
,
#include "shaders/lighting.glsl"
,
#include "shaders/lighting.glsl"
,
#include "shaders/default.vert"
,
#include "shaders/default.frag"
,
#include "shaders/printing.vert"
,
#include "shaders/printing.frag"
};

void initShaderState();

GLuint compileShaderFile(GLenum shaderType, const std::string& shaderText) {
    GLuint shader = glCreateShader(shaderType);
    GLint success = 0;
    int shader_len = shaderText.length();
    const char * shader_cstr = shaderText.c_str();
    glShaderSource(shader, 1, &shader_cstr, &shader_len);
    glCompileShader(shader);
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (success == GL_FALSE) {
        cerr << "Shader compilation failed." << endl;
        int err_len; 
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &err_len);
        char * error_text = new char[err_len];
        glGetShaderInfoLog(shader, err_len, &err_len, error_text);
        cerr << error_text << endl;
        delete [] error_text;
        return 0;
    }
    return shader;
}

template<int Count>
bool linkShaders(GLuint prgm, const GLuint (&shaders)[Count]) {
    // explicitly specify attrib positions so we don't have to reset VAO
    // bindings when switching programs

    // for OSX, attrib 0 must be bound to render an object
    glBindAttribLocation(prgm, 0, "vertex");
    glBindAttribLocation(prgm, 1, "textVertex");
    glBindAttribLocation(prgm, 2, "normal");
    glBindAttribLocation(prgm, 3, "color");
    glBindAttribLocation(prgm, 4, "texCoord0");
    glBindAttribLocation(prgm, 5, "texCoord1");
    for (int i = 0; i < Count; i++) {
        glAttachShader(prgm, shaders[i]);
    }
    glLinkProgram(prgm);
    GLint success = 0;
    glGetProgramiv(prgm, GL_LINK_STATUS, &success);
    if (success == GL_FALSE) {
        cerr << "FATAL: Shader linking failed." << endl;
    }
    return (success == GL_TRUE);
}

bool GlState::compileShaders() {
    GLuint refshaders[NUM_SHADERS];
    for (int i = 0; i < NUM_SHADERS; i++) {
        GLenum shader_type = (i % 2 == 0) ? GL_VERTEX_SHADER
                                          : GL_FRAGMENT_SHADER;
        refshaders[i] = compileShaderFile(shader_type, _glsl_add + shader_files[i]);
        if (refshaders[i] == 0)
            return false;
    }
    default_program = glCreateProgram();
    GLuint default_pipeline[] = {
        refshaders[VS_CLIP_PLANE],
        refshaders[VS_DEFAULT],
        refshaders[FS_LIGHTING],
        refshaders[FS_CLIP_PLANE],
        refshaders[FS_DEFAULT]
    };
    if (!linkShaders(default_program, default_pipeline)) {
        glDeleteProgram(default_program);
        default_program = 0;
        return false;
    }
    //TODO: enable a legacy path for opengl2.1 without ext_tranform_feedback?
    print_program = glCreateProgram();
    const char * xfrm_varyings[] = {
        "gl_Position",
        "fColor",
        "fClipCoord",
    };
    glTransformFeedbackVaryings(print_program, 3, xfrm_varyings,
                                GL_INTERLEAVED_ATTRIBS);
    GLuint print_pipeline[] = {
        refshaders[VS_LIGHTING],
        refshaders[VS_PRINTING],
        refshaders[FS_PRINTING]
    };
    if (!linkShaders(print_program, print_pipeline)) {
        glDeleteProgram(print_program);
        print_program = 0;
    }
    glUseProgram(default_program);
    initShaderState(default_program);
    if (global_vao == 0) {
        glGenVertexArrays(1, &global_vao);
    }
    glBindVertexArray(global_vao);
    return true;
}

void GlState::initShaderState(GLuint program) {
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

    locNumLights = glGetUniformLocation(program, "num_lights");
    locGlobalAmb = glGetUniformLocation(program, "g_ambient");

    locSpec = glGetUniformLocation(program, "material.specular");
    locShin = glGetUniformLocation(program, "material.shininess");

    for (int i = 0; i < MAX_LIGHTS; i++) {
        std::string location = "lights[" + std::to_string(i) + "].";
        locPosition[i] = glGetUniformLocation(program, (location + "position").c_str());
        locDiffuse[i] = glGetUniformLocation(program, (location + "diffuse").c_str());
        locSpecular[i] = glGetUniformLocation(program, (location + "specular").c_str());
    }

    //Texture unit 0: color palettes
    //Texture unit 1: font atlas
    GLuint locColorTex = glGetUniformLocation(program, "colorTex");
    GLuint locFontTex = glGetUniformLocation(program, "fontTex");
    glUniform1i(locColorTex, 0);
    glUniform1i(locFontTex, 1);
    locContainsText = glGetUniformLocation(program, "containsText");
    glUniform1i(locContainsText, GL_FALSE);
    locUseColorTex = glGetUniformLocation(program, "useColorTex");
    glUniform1i(locUseColorTex, GL_FALSE);
    _shaderMode = RENDER_COLOR;
    modelView.identity();
    projection.identity();
    loadMatrixUniforms();
#ifndef __EMSCRIPTEN__
#endif
}
