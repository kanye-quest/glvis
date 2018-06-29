#include "glstate.hpp"
#include <string>
#include <iostream>

using std::cerr;
using std::endl;

#ifdef __EMSCRIPTEN__
const std::string _glsl_ver = "";
#else
const std::string _glsl_ver = "#version 120\n";
#endif

const std::string vertex_shader_file = _glsl_ver +
R"(
precision highp float;

attribute vec3 vertex;
attribute vec2 textVertex;
attribute vec4 color;
attribute vec3 normal;
attribute vec2 texCoord0;
attribute vec2 texCoord1;

uniform bool containsText;

uniform mat4 modelViewMatrix;
uniform mat4 projectionMatrix;
uniform mat4 textProjMatrix;
uniform mat3 normalMatrix; 
 
varying vec3 fNormal; 
varying vec3 fPosition; 
varying vec4 fColor; 
varying vec2 fTexCoord; 
varying vec2 fFontTexCoord;
 
void main() 
{ 
    fNormal = normalize(normalMatrix * normal); 
    vec4 pos = modelViewMatrix * vec4(vertex, 1.0);
    fPosition = pos.xyz; 
    fColor = color; 
    fTexCoord = texCoord0.xy; 
    fFontTexCoord = texCoord1.xy;
    vec4 textOffset = textProjMatrix * vec4(textVertex, 0.0, 0.0);
    pos = projectionMatrix * pos;
    gl_Position = pos;
    if (containsText) {
        gl_Position += vec4((textOffset.xy * pos.w), -0.005, 0.0);
    }
})";

const std::string fragment_shader_file = _glsl_ver +
R"(
precision highp float;

uniform bool containsText; 
uniform bool useColorTex;
uniform bool useClipPlane;
 
uniform sampler2D fontTex; 
uniform sampler2D colorTex;
uniform vec4 clipPlane;
 
varying vec3 fNormal; 
varying vec3 fPosition; 
varying vec4 fColor; 
varying vec2 fTexCoord; 
varying vec2 fFontTexCoord; 
 
struct PointLight { 
    vec3 position; 
    vec4 diffuse; 
    vec4 specular; 
}; 
 
uniform int numLights; 
uniform PointLight lights[3]; 
uniform vec4 g_ambient; 
 
struct Material { 
    vec4 ambient; 
    vec4 diffuse; 
    vec4 specular; 
    float shininess; 
}; 
 
uniform Material material; 
 
void main() 
{
    if (useClipPlane && dot(vec4(fPosition, 1.0),clipPlane) < 0.0) {
        discard;
    }
    if (containsText) { 
        vec4 colorOut = vec4(0.0, 0.0, 0.0, texture2D(fontTex, fFontTexCoord).a); 
        if (colorOut.a < 0.01)
            discard;
        gl_FragColor = colorOut;
    } else { 
        vec4 color = fColor; 
        if (useColorTex) { 
            color = texture2D(colorTex, fTexCoord); 
        }
        if (numLights == 0) {
            gl_FragColor = color;
        } else {
            float normSgn = float(int(gl_FrontFacing) * 2 - 1);
            vec4 ambient_light = g_ambient * material.ambient; 
            vec4 diffuse_light = vec4(0.0, 0.0, 0.0, 0.0); 
            vec4 specular_light = vec4(0.0, 0.0, 0.0, 0.0); 
            for (int i = 0; i < 3; i++) {
                if (i == numLights)
                    break;
                vec3 light_dir = normalize(lights[i].position - fPosition); 
                diffuse_light += lights[i].diffuse * material.diffuse * max(dot(fNormal * normSgn, light_dir), 0.0); 
     
                //vec3 eye_to_vert = normalize(-fPosition); 
                vec3 half_v = normalize(vec3(0,0,1) + light_dir);
                float specular_factor = max(dot(half_v, fNormal * normSgn), 0.0); 
                specular_light += lights[i].specular * material.specular * pow(specular_factor, material.shininess); 
            } 
            gl_FragColor.xyz = vec3(color) * (ambient_light.xyz + diffuse_light.xyz + specular_light.xyz);
            gl_FragColor.w = 1.0;
        }
    } 
})";


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

    //glDetachShader(program, vtx_shader);
    //glDetachShader(program, frag_shader);
    //glDeleteShader(vtx_shader);
    //glDeleteShader(frag_shader);
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

    locAmb = glGetUniformLocation(program, "material.ambient");
    locDif = glGetUniformLocation(program, "material.diffuse");
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
}
