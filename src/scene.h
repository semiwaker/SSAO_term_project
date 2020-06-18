#pragma once
#ifndef SCENE_H
#define SCENE_H

#include <array>
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
    glm::vec3 tangent;
    glm::vec3 bitangent;
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
    void applyTrans(glm::mat4 trans);

private:
    void draw(DrawFunc func, glm::mat4 trans);
    std::vector<std::unique_ptr<Node>> children;
    std::vector<int> meshes;
    glm::mat4 transMat;
};

class Quad
{
public:
    Quad(int width, int height);
    Quad(const Quad &) = delete;
    Quad &operator=(const Quad &) = delete;
    Quad(Quad &&) = delete;
    Quad &operator=(Quad &&) = delete;
    ~Quad();

    void draw() const;

private:
    GLuint VAO{0};
    GLuint VBO{0};
};

class GBuffer
{
public:
    GBuffer(int width, int height);
    GBuffer(const GBuffer &) = delete;
    GBuffer &operator=(const GBuffer &) = delete;
    GBuffer(GBuffer &&) = delete;
    GBuffer &operator=(GBuffer &&) = delete;
    ~GBuffer();

    void bindForRender() const;
    void bindAsTextures() const;
    void unbind() const;

private:
    GLuint gBuffer{0};
    GLuint position{0};
    GLuint normal{0};
    GLuint albedo{0};
    GLuint light{0};
    GLuint rboDepth;
};

class SkyBox
{
public:
    SkyBox(const std::string &file, int screenWidth, int screenHeight);
    SkyBox(const SkyBox &) = delete;
    SkyBox &operator=(const SkyBox &) = delete;
    SkyBox(SkyBox &&) = delete;
    SkyBox &operator=(SkyBox &&) = delete;
    ~SkyBox();

    void render(glm::mat4 view, glm::mat4 proj) const;
    void bind(int pos) const;

private:
    Pipeline skybox;
    GLuint hdr;
    GLuint VAO;
    GLuint VBO;
    GLuint renderViewIndex;
    GLuint renderProjIndex;
    int width;
    int height;
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

    virtual void render(std::shared_ptr<Node> root, glm::mat4 proj, const Camera &camera) const = 0;
    virtual void setLight(glm::vec3 lightPos, glm::vec3 lightDir) const = 0;

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
    void render(std::shared_ptr<Node> root, glm::mat4 proj, const Camera &camera) const override;
    void render(int idx, glm::mat4 modelMat, glm::mat4 VPMat, glm::vec3 center) const;
    void setLight(glm::vec3 lightPos, glm::vec3 lightDir) const override;

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
class SSAORenderer : public Renderer
{
public:
    SSAORenderer(
        const std::vector<Mesh> &mesh,
        int width,
        int height,
        bool diffuseMap = false,
        bool specularMap = false,
        bool normalsMap = false,
        bool heightMap = false);
    SSAORenderer(const BaselineRenderer &) = delete;
    SSAORenderer &operator=(const BaselineRenderer &) = delete;
    SSAORenderer(BaselineRenderer &&) = delete;
    SSAORenderer &operator=(BaselineRenderer &&) = delete;
    ~SSAORenderer() override;
    void setLight(glm::vec3 lightPos, glm::vec3 lightDir) const override;

private:
    void makeSSAOFBO();
    void makeBlurFBO();
    void makeShadowFBO();
    void makeKernel();
    void makeNoise();

    void render(std::shared_ptr<Node> root, glm::mat4 proj, const Camera &camera) const override;
    void geometryPass(std::shared_ptr<Node> root, glm::mat4 viewMat, glm::mat4 projMat) const;
    void geometryRender(int idx, glm::mat4 modelMat, glm::mat4 viewMat, glm::mat4 projMat) const;
    void ssaoPass(glm::mat4 projMat) const;
    void blurPass() const;
    void shadowPass(std::shared_ptr<Node> root) const;
    void shadowRender(int idx, glm::mat4 modelMat) const;
    void lightingPass(glm::vec3 viewPos) const;
    void stencilPass(std::shared_ptr<Node> root, glm::mat4 viewMat, glm::mat4 projMat) const;
    void stencilRender(int idx, glm::mat4 modelMat, glm::mat4 viewMat, glm::mat4 projMat) const;

    Pipeline geometry;
    Pipeline ssao;
    Pipeline blur;
    Pipeline shadow;
    Pipeline lighting;
    Pipeline stencil;
    Quad quad;
    SkyBox skybox;
    GBuffer gBuffer;
    GLuint ssaoFBO;
    GLuint ssaoColorBuffer;
    GLuint blurFBO;
    GLuint blurBuffer;
    GLuint shadowFBO;
    GLuint shadowBuffer;
    GLuint noiseTexture;
    GLint WVPIndex;
    GLint WVIndex;
    GLint lightMatIndex;
    GLint lightDirIndex;
    GLint shadowModelMatIndex;
    GLint shadowLightMatIndex;
    GLint modelMatIndex;
    GLint projMatIndex;
    GLint lightPosIndex;
    GLint viewPosIndex;
    GLint shininessIndex;
    GLint shininessStrengthIndex;
    GLint stencilWVPIndex;
    bool _diffuseMap{false};
    bool _specularMap{false};
    bool _normalsMap{false};
    bool _heightMap{false};
    int _width;
    int _height;

    std::array<GLfloat, 64 * 3> kernel;
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
    std::shared_ptr<Node> root;
    std::map<std::string, Texture> loadedTextures;
    std::vector<Mesh> meshes;

    std::unique_ptr<Renderer> renderer;
};

GLuint TextureFromFile(const std::string &path, const std::string &directory, bool gamma);

#endif