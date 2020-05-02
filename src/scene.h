#pragma once
#ifndef SCENE_H
#define SCENE_H

#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "assimp/Importer.hpp"
#include "assimp/Scene.h"
#include "assimp/Postprocess.h"
#include "glm/glm.hpp"
#include "glm/ext.hpp"

#include "GLenv.h"

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

struct Vertex
{
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 texCoords;
};

struct Texture
{
    GLuint id;
    GLuint pos;
};

struct TextureTuple
{
    aiTextureType type;
    std::string name;
    GLuint pos;
};

const int TEXTURE_TYPE_CNT = 4;
const TextureTuple textureTypes[] = {
    TextureTuple{aiTextureType_DIFFUSE, "textureDiffuse"s, 0},
    TextureTuple{aiTextureType_SPECULAR, "textureSpecular"s, 1},
    TextureTuple{aiTextureType_NORMALS, "textureNormals"s, 2},
    TextureTuple{aiTextureType_HEIGHT, "textureHeight"s, 3},
};

class Scene;
class Mesh
{
public:
    Mesh(aiMesh *mesh, Scene *scene);
    ~Mesh();

    void bindVAO();
    void bindTexture(const std::string &name);

private:
    void setup();

    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    std::map<std::string, Texture> textures;
    GLuint VAO{0};
    GLuint VBO{0};
    GLuint EBO{0};
};

class Node
{
public:
    Node(aiNode *node);

    using DrawFunc = std::function<void(int, glm::mat4)>;

    void draw(DrawFunc func);

private:
    void draw(DrawFunc func, glm::mat4 trans);
    std::vector<std::unique_ptr<Node>> children;
    std::vector<int> meshes;
    glm::mat4 transMat;
};

class Scene
{
public:
    Scene(aiScene *scene, const std::string &directory);
    ~Scene();
    void render() const;

    std::map<std::string, Texture> loadMaterialTexures(unsigned int index);

private:
    aiScene *ai_scene;
    std::string dir;
    std::unique_ptr<Node> root;
    std::map<std::string, Texture> loadedTextures;
    std::vector<Mesh> meshes;
};

GLuint TextureFromFile(const string &path, const string &directory, bool gamma);

#endif