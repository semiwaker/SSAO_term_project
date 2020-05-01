#pragma once
#ifndef SCENE_H
#define SCENE_H

#include "GLenv.h"
#include "assimp/Importer.hpp"
#include "assimp/Scene.h"
#include "assimp/Postprocess.h"

#include <string>

class Shader
{
public:
    Shader(const std::string &fileName, GLenum shaderType);
    Shader(const Shader &) = delete;
    Shader &operator=(const Shader &) = delete;
    Shader(Shader &&);
    Shader &operator=(Shader &&);
    ~Shader();

    GLuint getShader() const;

private:
    GLenum _type{0};
    GLuint _obj{0};
};

class Pipeline
{
public:
    Pipeline();
    Pipeline(const Pipeline &) = delete;
    Pipeline &operator=(const Pipeline &) = delete;
    Pipeline(Pipeline &&) = delete;
    Pipeline &operator=(Pipeline &&) = delete;
    ~Pipeline() noexcept;

    void addShader(const Shader &shader) const;
    void link();
    GLint uniformLocation(const char *name) const;

    void use() const;

protected:
    GLuint _obj{0};
};

class Scene
{
public:
    void render() const;

private:
};

#endif