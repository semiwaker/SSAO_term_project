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
#include "camera.h"

class Shader
{
public:
    Shader(const std::string &fileName, GLenum shaderType);
    Shader(const Shader &) = delete;
    Shader &operator=(const Shader &) = delete;
    Shader(Shader &&);
    Shader &operator=(Shader &&);
    ~Shader();

    GLuint getShader() const noexcept;

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
    // glm::vec3 tangent;
    // glm::vec3 bitangent;
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
    TextureTuple{aiTextureType_DIFFUSE, "textureDiffuse", 0},
    TextureTuple{aiTextureType_SPECULAR, "textureSpecular", 1},
    TextureTuple{aiTextureType_NORMALS, "textureNormals", 2},
    TextureTuple{aiTextureType_HEIGHT, "textureHeight", 3},
};

struct MaterialParams
{
    std::map<std::string, float> floatParams;
};

class Scene;
class Mesh
{
public:
    Mesh(const aiMesh *mesh, Scene *scene);
    Mesh(const Mesh &) = delete;
    Mesh &operator=(const Mesh &) = delete;
    Mesh(Mesh &&);
    Mesh &operator=(Mesh &&);
    ~Mesh();

    void bindVAO() const;
    void bindTexture(const std::string &name) const;
    float getFloatParam(const std::string &name) const;
    void draw() const;

private:
    void setup();

    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    std::map<std::string, Texture> textures;
    MaterialParams params;
    GLuint VAO{0};
    GLuint VBO{0};
    GLuint EBO{0};
};

class Node
{
public:
    Node(const aiNode *node);

    using DrawFunc = std::function<void(int, glm::mat4)>;

    void draw(DrawFunc func);

private:
    void draw(DrawFunc func, glm::mat4 trans);
    std::vector<std::unique_ptr<Node>> children;
    std::vector<int> meshes;
    glm::mat4 transMat;
};

class Renderer
{
public:
    Renderer(const std::vector<Mesh> &mesh);
    Renderer(const Renderer &) = delete;
    Renderer &operator=(const Renderer &) = delete;
    Renderer(Renderer &&) = delete;
    Renderer &operator=(Renderer &&) = delete;
    virtual ~Renderer() = default;

    virtual void render(int idx, glm::mat4 modelMat, glm::mat4 VPMat, glm::vec3 center) const = 0;
    virtual void setLightPos(glm::vec3 lightPos) const = 0;

protected:
    const std::vector<Mesh> &meshes;
};

class BaselineRenderer : public Renderer
{
public:
    BaselineRenderer(
        const std::vector<Mesh> &mesh,
        bool diffuseMap = false,
        bool specularMap = false,
        bool normalsMap = false,
        bool heightMap = false,
        const std::string &vertexShader = "baseline.vs",
        const std::string &fragmentShader = "baseline.fs");
    BaselineRenderer(const BaselineRenderer &) = delete;
    BaselineRenderer &operator=(const BaselineRenderer &) = delete;
    BaselineRenderer(BaselineRenderer &&) = delete;
    BaselineRenderer &operator=(BaselineRenderer &&) = delete;
    ~BaselineRenderer() override;
    void render(int idx, glm::mat4 modelMat, glm::mat4 VPMat, glm::vec3 center) const override;
    void setLightPos(glm::vec3 lightPos) const override;

private:
    Pipeline pipeline;
    GLint modelMatIndex;
    GLint VPMatIndex;
    GLint lightPosIndex;
    GLint viewPosIndex;
    GLint shininessIndex;
    GLint shininessStrengthIndex;
    bool _diffuseMap{false};
    bool _specularMap{false};
    bool _normalsMap{false};
    bool _heightMap{false};
};

class Scene
{
public:
    Scene(const aiScene *scene, const std::string &directory);
    Scene(const Scene &) = delete;
    Scene &operator=(const Scene &) = delete;
    Scene(Scene &&) = delete;
    Scene &operator=(Scene &&) = delete;
    ~Scene();

    void render(glm::mat4 proj, const Camera &camera) const;

    std::map<std::string, Texture> loadMaterialTexures(unsigned int index);
    MaterialParams loadMaterialParams(unsigned int index);

private:
    const aiScene *ai_scene;
    std::string dir;
    std::unique_ptr<Node> root;
    std::map<std::string, Texture> loadedTextures;
    std::vector<Mesh> meshes;

    std::unique_ptr<Renderer> renderer;
};

GLuint TextureFromFile(const std::string &path, const std::string &directory, bool gamma);

#endif