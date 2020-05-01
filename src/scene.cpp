#include <fstream>
#include <iostream>

#include "scene.h"
#include "utils.h"

using namespace std;

Shader::Shader(const string &fileName, GLenum shaderType)
    : _type(shaderType)
{
    _obj = glCreateShader(shaderType);
    ScopeGuard gObj{[this]() {
        glDeleteShader(_obj);
        _obj = 0;
    }};

    const string shaderCode = loadFile(fileName);
    GLint isCompiled{0};
    const char *code = shaderCode.c_str();
    glShaderSource(_obj, 1, &code, NULL);
    glCompileShader(_obj);
    glGetShaderiv(_obj, GL_COMPILE_STATUS, &isCompiled);
    if (GL_FALSE == isCompiled)
    {
        GLint maxLength{0};
        glGetShaderiv(_obj, GL_INFO_LOG_LENGTH, &maxLength);

        string log;
        log.resize(maxLength);
        glGetShaderInfoLog(_obj, maxLength, &maxLength, log.data());

        cerr << log << endl;
        exit(1);
    }

    gObj.commit();
}
Shader::Shader(Shader &&otherShader)
    : _type(otherShader._type), _obj(otherShader._obj)
{
    otherShader._obj = 0;
}
Shader &Shader::operator=(Shader &&otherShader)
{
    _type = otherShader._type;
    _obj = otherShader._obj;
    otherShader._obj = 0;
    return *this;
}
Shader::~Shader() noexcept
{
    if (_obj)
        glDeleteShader(_obj);
}
GLuint Shader::getShader() const noexcept
{
    return _obj;
}

Pipeline::Pipeline()
    : _obj(glCreateProgram())
{
}

Pipeline::~Pipeline() noexcept
{
    if (_obj)
        glDeleteProgram(_obj);
}

void Pipeline::addShader(const Shader &shader) const
{
    glAttachShader(_obj, shader.getShader());
}

void Pipeline::link()
{
    glLinkProgram(_obj);

    GLint isLinked{0};
    glGetProgramiv(_obj, GL_LINK_STATUS, &isLinked);
    if (!isLinked)
    {
        int maxLength{0};
        glGetProgramiv(_obj, GL_INFO_LOG_LENGTH, &maxLength);

        string log;
        log.resize(maxLength);
        glGetProgramInfoLog(_obj, maxLength, &maxLength, log.data());

        glDeleteProgram(_obj);
        _obj = 0;

        cerr << log << endl;
        exit(1);
    }
}

void Pipeline::use() const
{
    glUseProgram(_obj);
}
GLint Pipeline::uniformLocation(const char *name) const
{
    assert(_obj, "Empty object.");
    return glGetUniformLocation(_obj, name);
}