#include <fstream>
#include <iostream>

#include "scene.h"
#include "utils.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

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

Mesh::Mesh(aiMesh *mesh, Scene *scene)
{
    for (unsigned int i = 0; i < mesh->mNumVertices; ++i)
    {
        Vertex v;
        v.position = glm::vec3{mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z};
        v.normal = glm::vec3{mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z};
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

    setup();
}
void Mesh::setup()
{
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

    glBindVertexArray(0);
}
Mesh::~Mesh()
{
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &EBO);
    glDeleteBuffers(1, &VBO);
}
void Mesh::bindVAO()
{
    glBindVertexArray(VAO);
}
void Mesh::bindTexture(const std::string &name)
{
    auto texture = textures.find(name);
    assert(texture != textures.end());
    glActiveTexture(GL_TEXTURE0 + texture->second.pos);
    glBindTexture(GL_TEXTURE_2D, texture->second.id);
}
Node::Node(aiNode *node)
    : meshes(node->mMeshes, node->mMeshes + node->mNumMeshes)
{
    for (int i = 0; i != node->mNumChildren; ++i)
        children.emplace_back(std::make_unique<Node>(node->mChildren[i]));
    transMat = glm::make_mat4(&node->mTransformation);
}
void Node::draw(DrawFunc func)
{
    draw(func, glm::mat4());
}
void Node::draw(DrawFunc func, glm::mat4 trans)
{
    trans = transMat * trans;
    for (auto i : meshes)
        func(i, trans);
    for (auto &i : children)
        i->draw(func, trans);
}

Scene::Scene(aiScene *scene, const std::string &directory) : ai_scene(scene), dir(directory)
{
    int meshCnt = scene->mNumMeshes;
    for (int i = 0; i != meshCnt; ++i)
        meshes.push_back(Mesh{scene->mMeshes[i], this});
    root = make_unique<Node>(scene->mRootNode);
}
Scene::~Scene()
{
    for (auto &t : loadedTextures)
        glDeleteTextures(1, &(t.second.id));
}

void Scene::render() const
{
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
            result.insert(*iter);
        else
        {
            Texture t{TextureFromFile(fileName, dir, false), textureTypes[i].pos};
            result[textureTypes[i].name] = t;
            loadedTextures[textureTypes[i].name] = t;
        }
    }
    return result;
}

GLuint TextureFromFile(const string &path, const string &directory, bool gamma)
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

        stbi_image_free(data);
    }
    else
    {
        std::cerr << "Texture failed to load at path: " << path << std::endl;
        stbi_image_free(data);
    }

    return textureID;
}