#include <algorithm>
#include <fstream>
#include <iostream>

#include "scene.h"
#include "utils.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

using namespace std;

#define DEBUG

#ifdef DEBUG
#define CHECKERROR(msg)                \
    if (GL_NO_ERROR != glGetError())   \
    {                                  \
        std::cerr << msg << std::endl; \
        exit(1);                       \
    }
#else
#define CHECKERROR(msg)
#endif

Shader::Shader(const std::string &fileName, GLenum shaderType)
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

        std::cerr << log << std::endl;
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
    // const GLchar *feedback[] = {"gl_Position"};
    // glTransformFeedbackVaryings(_obj, 1, feedback, GL_INTERLEAVED_ATTRIBS);
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

        std::cerr << log << std::endl;
        exit(1);
    }
}

void Pipeline::use() const
{
    glUseProgram(_obj);
}
GLint Pipeline::uniformLocation(const char *name) const
{
    assert(_obj);
    return glGetUniformLocation(_obj, name);
}

Mesh::Mesh(const aiMesh *mesh, Scene *scene)
{
    std::cout << "Loading Mesh" << std::endl;
    for (unsigned int i = 0; i < mesh->mNumVertices; ++i)
    {
        Vertex v;
        v.position = glm::vec3{mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z};
        v.normal = glm::vec3{mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z};
        // v.tangent = glm::vec3{mesh->mTangents[i].x, mesh->mTangents[i].y, mesh->mTangents[i].z};
        // v.bitangent = glm::vec3{mesh->mBitangents[i].x, mesh->mBitangents[i].y, mesh->mBitangents[i].z};
        if (mesh->HasTextureCoords(0))
            v.texCoords = glm::vec2{mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y};
        else
            v.texCoords = glm::vec2{0.0f, 0.0f};
        vertices.push_back(v);
    }

    for (unsigned int i = 0; i < mesh->mNumFaces; ++i)
    {
        auto face = mesh->mFaces[i];
        for (unsigned int j = 0; j < face.mNumIndices; j++)
            indices.push_back(face.mIndices[j]);
    }
    textures = scene->loadMaterialTexures(mesh->mMaterialIndex);
    params = scene->loadMaterialParams(mesh->mMaterialIndex);

    setup();
    std::cout << "Mesh Loaded" << std::endl;
}
void Mesh::setup()
{
    // float max_x{vertices[0].position.x};
    // float max_y{vertices[0].position.y};
    // float max_z{vertices[0].position.z};
    // float min_x{vertices[0].position.x};
    // float min_y{vertices[0].position.y};
    // float min_z{vertices[0].position.z};
    // for (auto &i : vertices)
    // {
    //     max_x = std::max(max_x, i.position.x);
    //     max_y = std::max(max_y, i.position.y);
    //     max_z = std::max(max_z, i.position.z);
    //     min_x = std::min(min_x, i.position.x);
    //     min_y = std::min(min_y, i.position.y);
    //     min_z = std::min(min_z, i.position.z);
    // }
    // std::cerr << max_x << " " << max_y << " " << max_z << std::endl;
    // std::cerr << min_x << " " << min_y << " " << min_z << std::endl;

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);

    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), &vertices[0], GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int),
                 &indices[0], GL_STATIC_DRAW);

    // vertex positions
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)0);
    // vertex normals
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)offsetof(Vertex, normal));
    // vertex texture coords
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)offsetof(Vertex, texCoords));
    /*
    // vertex tangent coords
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)offsetof(Vertex, tangent));
    // vertex bitangent coords
    glEnableVertexAttribArray(4);
    glVertexAttribPointer(4, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)offsetof(Vertex, bitangent));
    */

    glBindVertexArray(0);
}
Mesh::Mesh(Mesh &&other)
    : vertices(other.vertices), indices(other.indices), textures(other.textures),
      params(other.params), VAO(other.VAO), VBO(other.VBO), EBO(other.EBO)
{
    other.vertices.clear();
    other.indices.clear();
    other.textures.clear();
    other.VAO = 0;
    other.VBO = 0;
    other.EBO = 0;
}
Mesh &Mesh::operator=(Mesh &&other)
{
    vertices = other.vertices;
    indices = other.indices;
    textures = other.textures;
    params = other.params;
    VAO = other.VAO;
    VBO = other.VBO;
    EBO = other.EBO;
    other.vertices.clear();
    other.indices.clear();
    other.textures.clear();
    other.VAO = 0;
    other.VBO = 0;
    other.EBO = 0;
    return *this;
}
Mesh::~Mesh()
{
    if (VAO)
        glDeleteVertexArrays(1, &VAO);
    if (EBO)
        glDeleteBuffers(1, &EBO);
    if (VBO)
        glDeleteBuffers(1, &VBO);
}
void Mesh::bindVAO() const
{
    glBindVertexArray(VAO);
}
void Mesh::bindTexture(const std::string &name) const
{
    auto texture = textures.find(name);
    assert(texture != textures.end());
    glActiveTexture(GL_TEXTURE0 + texture->second.pos);
    glBindTexture(GL_TEXTURE_2D, texture->second.id);
}
float Mesh::getFloatParam(const std::string &name) const
{
    return params.floatParams.at(name);
}
void Mesh::draw() const
{
    // GLuint tbo;
    // glGenBuffers(1, &tbo);
    // glBindBuffer(GL_ARRAY_BUFFER, tbo);
    // glBufferData(GL_ARRAY_BUFFER, indices.size() * 3, nullptr, GL_STATIC_READ);

    // glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, tbo);

    // glBeginTransformFeedback(GL_TRIANGLES);

    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(indices.size()), GL_UNSIGNED_INT, 0);

    // glEndTransformFeedback();
    // glFlush();

    // GLfloat *feedback = new GLfloat[indices.size() * 3];
    // memset(feedback, 0, indices.size() * 3 * sizeof(GLfloat));
    // glGetBufferSubData(GL_TRANSFORM_FEEDBACK_BUFFER, 0, indices.size() * 3, feedback);
    // for (int i = 0; i != indices.size(); ++i)
    //     std::cerr << feedback[i * 3] << " " << feedback[i * 3 + 1] << " " << feedback[i * 3 + 2] << std::endl;
    // delete[] feedback;
}
Node::Node(const aiNode *node)
    : meshes(node->mMeshes, node->mMeshes + node->mNumMeshes)
{
    std::cout << "Loading Node" << node->mName.C_Str() << std::endl;
    for (int i = 0; i != node->mNumChildren; ++i)
        children.emplace_back(std::make_unique<Node>(node->mChildren[i]));
    for (int i = 0; i != 4; ++i)
        for (int j = 0; j != 4; ++j)
            transMat[i][j] = node->mTransformation[i][j];
}
void Node::draw(DrawFunc func)
{
    draw(func, glm::rotate(glm::identity<glm::mat4>(), glm::radians(90.0f), glm::vec3(1.0, 0.0, 0.0)));
}
void Node::draw(DrawFunc func, glm::mat4 trans)
{
    trans = transMat * trans;
    for (auto i : meshes)
        func(i, trans);
    for (auto &i : children)
        i->draw(func, trans);
}
Renderer::Renderer(const std::vector<Mesh> &mesh) : meshes(mesh)
{
}
BaselineRenderer::BaselineRenderer(
    const std::vector<Mesh> &mesh,
    bool diffuseMap,
    bool specularMap,
    bool normalsMap,
    bool heightMap,
    const string &vertexShader,
    const string &fragmentShader)
    : Renderer(mesh),
      _diffuseMap(diffuseMap), _specularMap(specularMap), _normalsMap(normalsMap), _heightMap(heightMap)
{
    Shader baselineVs("shaders/"s + vertexShader, GL_VERTEX_SHADER);
    Shader baselineFs("shaders/"s + fragmentShader, GL_FRAGMENT_SHADER);
    pipeline.addShader(baselineVs);
    pipeline.addShader(baselineFs);
    pipeline.link();

    pipeline.use();
    glUniform3f(pipeline.uniformLocation("lightAmbient"), 1.0f, 1.0f, 1.0f);
    glUniform3f(pipeline.uniformLocation("lightDiffuse"), 1.0f, 1.0f, 1.0f);
    glUniform3f(pipeline.uniformLocation("lightSpecular"), 1.0f, 1.0f, 1.0f);
    modelMatIndex = pipeline.uniformLocation("modelMat");
    VPMatIndex = pipeline.uniformLocation("VPMat");
    lightPosIndex = pipeline.uniformLocation("lightPos");
    viewPosIndex = pipeline.uniformLocation("viewPos");
    shininessIndex = pipeline.uniformLocation("shininess");
    shininessStrengthIndex = pipeline.uniformLocation("shininessStrength");

    if (diffuseMap)
        glUniform1i(pipeline.uniformLocation("textureDiffuse"), 0);
    if (normalsMap)
        glUniform1i(pipeline.uniformLocation("textureNormals"), 0);
    if (specularMap)
        glUniform1i(pipeline.uniformLocation("textureSpecular"), 0);
    if (heightMap)
        glUniform1i(pipeline.uniformLocation("textureHeight"), 0);
}
BaselineRenderer::~BaselineRenderer()
{
}
void BaselineRenderer::setLightPos(glm::vec3 lightPos) const
{
    glUniform3f(lightPosIndex, lightPos.x, lightPos.y, lightPos.z);
}

void BaselineRenderer::render(int idx, glm::mat4 modelMat, glm::mat4 VPMat, glm::vec3 center) const
{
    pipeline.use();
    meshes[idx].bindVAO();
    CHECKERROR("BindVAO Error");
    if (_diffuseMap)
        meshes[idx].bindTexture("textureDiffuse");
    if (_specularMap)
        meshes[idx].bindTexture("textureSpecular");
    if (_normalsMap)
        meshes[idx].bindTexture("textureNormals");
    if (_heightMap)
        meshes[idx].bindTexture("textureHeight");

    CHECKERROR("BindTexture Error");

    glUniformMatrix4fv(modelMatIndex, 1, false, glm::value_ptr(modelMat));
    CHECKERROR("modelMat Error");
    glUniformMatrix4fv(VPMatIndex, 1, false, glm::value_ptr(VPMat));
    CHECKERROR("VPMat Error");
    glUniform3f(viewPosIndex, center.x, center.y, center.z);
    CHECKERROR("viewPos Error");

    glUniform1f(shininessIndex, meshes[idx].getFloatParam("shininess"));
    // glUniform1f(shininessStrengthIndex, meshes[idx].getFloatParam("shininessStrength"));

    meshes[idx].draw();
    CHECKERROR("Draw Error");
}

Scene::Scene(const aiScene *scene, const std::string &directory) : ai_scene(scene), dir(directory)
{
    int meshCnt = scene->mNumMeshes;
    std::cout << "Load model" << std::endl;
    for (int i = 0; i != meshCnt; ++i)
        meshes.emplace_back(scene->mMeshes[i], this);
    root = make_unique<Node>(scene->mRootNode);

    renderer = make_unique<BaselineRenderer>(meshes, false, false, false, false, "baseline.vs", "baseline.fs");
    renderer->setLightPos(glm::vec3{100.0f, 0.0f, -100.0f});
}
Scene::~Scene()
{
    for (auto &t : loadedTextures)
        glDeleteTextures(1, &(t.second.id));
}

void Scene::render(glm::mat4 proj, const Camera &camera) const
{
    proj = proj * camera.getTransMat();
    glm::vec3 center = camera.center();
    root->draw([this, &proj, &center](int idx, glm::mat4 trans) {
        renderer->render(idx, trans, proj, center);
    });
}

std::map<std::string, Texture> Scene::loadMaterialTexures(unsigned int index)
{
    assert(index < ai_scene->mNumMaterials);

    auto material = ai_scene->mMaterials[index];
    std::map<std::string, Texture> result;
    for (int i = 0; i != TEXTURE_TYPE_CNT; ++i)
    {
        int cnt = material->GetTextureCount(textureTypes[i].type);
        if (cnt == 0)
            continue;
        aiString aifileName;
        material->GetTexture(textureTypes[i].type, 0, &aifileName);
        std::string fileName{aifileName.C_Str()};
        auto iter = loadedTextures.find(fileName);
        if (iter != loadedTextures.end())
            result[textureTypes[i].name] = iter->second;
        else
        {
            std::cout << "Loading Texure: " << fileName << " Type: " << textureTypes[i].name << std::endl;
            Texture t{TextureFromFile(fileName, dir, false), textureTypes[i].pos};
            result[textureTypes[i].name] = t;
            loadedTextures[fileName] = t;
        }
    }
    return result;
}
MaterialParams Scene::loadMaterialParams(unsigned int index)
{
    MaterialParams result;
    auto material = ai_scene->mMaterials[index];
    float shininess{0.0f};
    material->Get(AI_MATKEY_SHININESS, shininess);
    result.floatParams["shininess"] = shininess;
    float shininessStrength{1.0f};
    material->Get(AI_MATKEY_SHININESS_STRENGTH, shininessStrength);
    result.floatParams["shininessStrength"] = shininessStrength;
    return result;
}

GLuint TextureFromFile(const std::string &path, const std::string &directory, bool gamma)
{
    auto filename = directory + '/' + path;

    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    unsigned char *data = stbi_load(filename.c_str(), &width, &height, &nrComponents, 0);
    if (data)
    {
        GLenum format;
        if (nrComponents == 1)
            format = GL_RED;
        else if (nrComponents == 3)
            format = GL_RGB;
        else if (nrComponents == 4)
            format = GL_RGBA;

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        CHECKERROR("LoadTexure");

        stbi_image_free(data);
    }
    else
    {
        std::cerr << "Texture failed to load at path: " << path << std::endl;
        stbi_image_free(data);
    }

    return textureID;
}