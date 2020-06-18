#include <algorithm>
#include <array>
#include <fstream>
#include <iostream>
#include <random>

#include "scene.h"
#include "utils.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

using namespace std;

const int shadowMapSize = 4096;

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
        v.tangent = glm::vec3{mesh->mTangents[i].x, mesh->mTangents[i].y, mesh->mTangents[i].z};
        v.bitangent = glm::vec3{mesh->mBitangents[i].x, mesh->mBitangents[i].y, mesh->mBitangents[i].z};
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
    // /*
    // vertex tangent coords
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)offsetof(Vertex, tangent));
    // vertex bitangent coords
    glEnableVertexAttribArray(4);
    glVertexAttribPointer(4, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)offsetof(Vertex, bitangent));
    // */

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
void Node::applyTrans(glm::mat4 trans)
{
    transMat = trans * transMat;
}

Quad::Quad(int width, int height)
{
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);

    std::array<glm::vec2, 4> vertices = {
        glm::vec2{-1.0, 1.0},
        glm::vec2{-1.0, -1.0},
        glm::vec2{1.0, 1.0},
        glm::vec2{1.0, -1.0}};

    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec2), vertices.data(), GL_STATIC_DRAW);

    // pos
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec2), (void *)0);
}
Quad::~Quad()
{
    if (VAO)
        glDeleteVertexArrays(1, &VAO);
    if (VBO)
        glDeleteBuffers(1, &VBO);
}
void Quad::draw() const
{
    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
}

GBuffer::GBuffer(int width, int height)
{
    glGenFramebuffers(1, &gBuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, gBuffer);

    // - position color buffer
    glGenTextures(1, &position);
    glBindTexture(GL_TEXTURE_2D, position);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, position, 0);

    // - normal color buffer
    glGenTextures(1, &normal);
    glBindTexture(GL_TEXTURE_2D, normal);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, normal, 0);

    // - color + specular color buffer
    glGenTextures(1, &albedo);
    glBindTexture(GL_TEXTURE_2D, albedo);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, albedo, 0);

    glGenTextures(1, &light);
    glBindTexture(GL_TEXTURE_2D, light);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, width, height, 0, GL_RED, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT3, GL_TEXTURE_2D, light, 0);

    glGenRenderbuffers(1, &rboDepth);
    glBindRenderbuffer(GL_RENDERBUFFER, rboDepth);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, width, height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rboDepth);

    // - tell OpenGL which color attachments we'll use (of this framebuffer) for rendering
    unsigned int attachments[4] = {
        GL_COLOR_ATTACHMENT0,
        GL_COLOR_ATTACHMENT1,
        GL_COLOR_ATTACHMENT2,
        GL_COLOR_ATTACHMENT3};
    glDrawBuffers(4, attachments);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cerr << "Framebuffer not complete!" << std::endl;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}
GBuffer::~GBuffer()
{
    glDeleteTextures(1, &position);
    glDeleteTextures(1, &normal);
    glDeleteTextures(1, &albedo);
    glDeleteTextures(1, &light);
    glDeleteRenderbuffers(1, &rboDepth);
    glDeleteFramebuffers(1, &gBuffer);
}
void GBuffer::bindForRender() const
{
    glBindFramebuffer(GL_FRAMEBUFFER, gBuffer);
}
void GBuffer::bindAsTextures() const
{
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, position);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, normal);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, albedo);
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, light);
}
void GBuffer::unbind() const
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

SkyBox::SkyBox(const std::string &name, int screenWidth, int screenHeight)
    : width(screenWidth), height(screenHeight)
{
    int width, height, nrComponents;
    float *data = stbi_loadf(name.c_str(), &width, &height, &nrComponents, 0);
    if (data)
    {
        glGenTextures(1, &hdr);
        glBindTexture(GL_TEXTURE_2D, hdr);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, width, height, 0, GL_RGB, GL_FLOAT, data);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
    }
    else
    {
        std::cerr << "Failed to load HDR image." << std::endl;
    }
    CHECKERROR("SkyBox HDRMap");

    glGenFramebuffers(1, &FBO);
    glGenRenderbuffers(1, &RBO);

    glBindFramebuffer(GL_FRAMEBUFFER, FBO);
    glBindRenderbuffer(GL_RENDERBUFFER, RBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 512, 512);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, RBO);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    CHECKERROR("SkyBox FrameBuffer");

    glGenTextures(1, &cubeMap);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubeMap);
    for (unsigned int i = 0; i < 6; ++i)
    {
        // note that we store each face with 16 bit floating point values
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F,
                     512, 512, 0, GL_RGB, GL_FLOAT, nullptr);
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    CHECKERROR("SkyBox CubeMap");

    Shader captureVS("shaders/capture.vs", GL_VERTEX_SHADER);
    Shader captureFS("shaders/capture.fs", GL_FRAGMENT_SHADER);
    capture.addShader(captureVS);
    capture.addShader(captureFS);
    capture.link();
    capture.use();
    glUniform1i(capture.uniformLocation("hdr"), 0);
    captureViewIndex = capture.uniformLocation("view");
    captureProjIndex = capture.uniformLocation("proj");

    Shader skyboxVS("shaders/skybox.vs", GL_VERTEX_SHADER);
    Shader skyboxFS("shaders/skybox.fs", GL_FRAGMENT_SHADER);
    skybox.addShader(skyboxVS);
    skybox.addShader(skyboxFS);
    skybox.link();
    skybox.use();
    glUniform1i(skybox.uniformLocation("skybox"), 0);
    renderViewIndex = skybox.uniformLocation("view");
    renderProjIndex = skybox.uniformLocation("proj");

    CHECKERROR("SkyBox Pipelines");

    float skyboxVertices[] = {
        -1.0f, 1.0f, -1.0f,
        -1.0f, -1.0f, -1.0f,
        1.0f, -1.0f, -1.0f,
        1.0f, -1.0f, -1.0f,
        1.0f, 1.0f, -1.0f,
        -1.0f, 1.0f, -1.0f,

        -1.0f, -1.0f, 1.0f,
        -1.0f, -1.0f, -1.0f,
        -1.0f, 1.0f, -1.0f,
        -1.0f, 1.0f, -1.0f,
        -1.0f, 1.0f, 1.0f,
        -1.0f, -1.0f, 1.0f,

        1.0f, -1.0f, -1.0f,
        1.0f, -1.0f, 1.0f,
        1.0f, 1.0f, 1.0f,
        1.0f, 1.0f, 1.0f,
        1.0f, 1.0f, -1.0f,
        1.0f, -1.0f, -1.0f,

        -1.0f, -1.0f, 1.0f,
        -1.0f, 1.0f, 1.0f,
        1.0f, 1.0f, 1.0f,
        1.0f, 1.0f, 1.0f,
        1.0f, -1.0f, 1.0f,
        -1.0f, -1.0f, 1.0f,

        -1.0f, 1.0f, -1.0f,
        1.0f, 1.0f, -1.0f,
        1.0f, 1.0f, 1.0f,
        1.0f, 1.0f, 1.0f,
        -1.0f, 1.0f, 1.0f,
        -1.0f, 1.0f, -1.0f,

        -1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f, 1.0f,
        1.0f, -1.0f, -1.0f,
        1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f, 1.0f,
        1.0f, -1.0f, 1.0f};

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);

    glBufferData(GL_ARRAY_BUFFER, 6 * 6 * sizeof(glm::vec3), skyboxVertices, GL_STATIC_DRAW);

    // pos
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void *)0);
    glBindVertexArray(0);
    CHECKERROR("SkyBox VAO");
}
SkyBox::~SkyBox()
{
    glDeleteRenderbuffers(1, &RBO);
    glDeleteFramebuffers(1, &FBO);
    glDeleteTextures(1, &hdr);
    glDeleteTextures(1, &cubeMap);
    glDeleteBuffers(1, &VBO);
    glDeleteVertexArrays(1, &VAO);
}
void SkyBox::render(glm::mat4 view, glm::mat4 proj) const
{
    skybox.use();
    glUniformMatrix4fv(renderProjIndex, 1, false, glm::value_ptr(proj));
    glUniformMatrix4fv(renderViewIndex, 1, false, glm::value_ptr(view));
    glBindVertexArray(VAO);
    glDepthFunc(GL_LEQUAL);
    bindCubeMap(0);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
    glDepthFunc(GL_LESS);
    CHECKERROR("SkyBox Render");
}
void SkyBox::prepare(glm::mat4 proj) const
{
    glm::mat4 captureProjection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
    glm::mat4 captureViews[] =
        {
            glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
            glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
            glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
            glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f)),
            glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
            glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, -1.0f, 0.0f))};

    // convert HDR equirectangular environment map to cubemap equivalent
    capture.use();
    glUniformMatrix4fv(captureProjIndex, 1, false, glm::value_ptr(proj));
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, hdr);

    glViewport(0, 0, 512, 512);
    glBindFramebuffer(GL_FRAMEBUFFER, FBO);
    glBindVertexArray(VAO);
    for (unsigned int i = 0; i < 6; ++i)
    {
        glUniformMatrix4fv(captureViewIndex, 1, false, glm::value_ptr(captureViews[i]));
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                               GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, cubeMap, 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glDrawArrays(GL_TRIANGLES, 0, 36);
    }
    glBindVertexArray(0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, width, height);
    CHECKERROR("SkyBox Prepare");
}
void SkyBox::bindCubeMap(int pos) const
{
    glActiveTexture(GL_TEXTURE0 + pos);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubeMap);
    CHECKERROR("SkyBox bindCubeMap");
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
    if (specularMap)
        glUniform1i(pipeline.uniformLocation("textureSpecular"), 1);
    if (normalsMap)
        glUniform1i(pipeline.uniformLocation("textureNormals"), 2);
    if (heightMap)
        glUniform1i(pipeline.uniformLocation("textureHeight"), 3);
}
BaselineRenderer::~BaselineRenderer()
{
}
void BaselineRenderer::setLight(glm::vec3 lightPos, glm::vec3 lightDir) const
{
    glUniform3f(lightPosIndex, lightPos.x, lightPos.y, lightPos.z);
}

void BaselineRenderer::render(std::shared_ptr<Node> root, glm::mat4 proj, const Camera &camera) const
{
    proj = proj * camera.getTransMat();
    glm::vec3 center = camera.center();
    root->draw([this, &proj, &center](int idx, glm::mat4 trans) {
        render(idx, trans, proj, center);
    });
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

SSAORenderer::SSAORenderer(
    const std::vector<Mesh> &mesh,
    int width,
    int height,
    bool diffuseMap,
    bool specularMap,
    bool normalsMap,
    bool heightMap)
    : Renderer(mesh),
      gBuffer(width, height), quad(width, height), skybox("model/table_mountain_1_2k.hdr", width, height),
      _diffuseMap(diffuseMap), _specularMap(specularMap), _normalsMap(normalsMap), _heightMap(heightMap),
      _width(width), _height(height)
{
    Shader geometryVS("shaders/geometry.vs"s, GL_VERTEX_SHADER);
    Shader geometryFS("shaders/geometry.fs"s, GL_FRAGMENT_SHADER);
    geometry.addShader(geometryVS);
    geometry.addShader(geometryFS);
    geometry.link();

    geometry.use();
    WVPIndex = geometry.uniformLocation("WVP");
    WVIndex = geometry.uniformLocation("WV");
    modelMatIndex = geometry.uniformLocation("modelMat");
    lightMatIndex = geometry.uniformLocation("lightMat");
    lightDirIndex = geometry.uniformLocation("lightDir");
    shininessIndex = geometry.uniformLocation("shininess");
    if (diffuseMap)
        glUniform1i(geometry.uniformLocation("textureDiffuse"), 0);
    if (specularMap)
        glUniform1i(geometry.uniformLocation("textureSpecular"), 1);
    if (normalsMap)
        glUniform1i(geometry.uniformLocation("textureNormals"), 2);
    if (heightMap)
        glUniform1i(geometry.uniformLocation("textureHeight"), 3);
    glUniform1i(geometry.uniformLocation("textureShadow"), 4);
    CHECKERROR("geometry");

    Shader quadVS("shaders/quad.vs"s, GL_VERTEX_SHADER);
    Shader SSAOFS("shaders/SSAO.fs"s, GL_FRAGMENT_SHADER);
    ssao.addShader(quadVS);
    ssao.addShader(SSAOFS);
    ssao.link();
    ssao.use();
    makeSSAOFBO();
    makeKernel();
    makeNoise();
    glUniform1i(ssao.uniformLocation("texturePosition"), 0);
    glUniform1i(ssao.uniformLocation("textureNormal"), 1);
    glUniform1i(ssao.uniformLocation("textureAlbedo"), 2);
    glUniform1i(ssao.uniformLocation("textureLight"), 3);
    glUniform1i(ssao.uniformLocation("textureNoise"), 4);
    glUniform3fv(ssao.uniformLocation("kernel"), 64, kernel.data());
    projMatIndex = ssao.uniformLocation("projMat");
    CHECKERROR("SSAO");

    Shader blurFS("shaders/blur.fs"s, GL_FRAGMENT_SHADER);
    blur.addShader(quadVS);
    blur.addShader(blurFS);
    blur.link();
    blur.use();
    glUniform1i(blur.uniformLocation("textureSSAO"), 0);
    makeBlurFBO();

    Shader shadowVS("shaders/shadow.vs", GL_VERTEX_SHADER);
    Shader shadowFS("shaders/shadow.fs", GL_FRAGMENT_SHADER);
    shadow.addShader(shadowVS);
    shadow.addShader(shadowFS);
    shadow.link();
    shadow.use();
    makeShadowFBO();
    shadowModelMatIndex = shadow.uniformLocation("modelMat");
    shadowLightMatIndex = shadow.uniformLocation("lightMat");

    Shader lightingFS("shaders/lighting.fs"s, GL_FRAGMENT_SHADER);
    lighting.addShader(quadVS);
    lighting.addShader(lightingFS);
    lighting.link();

    lighting.use();
    glUniform1i(lighting.uniformLocation("texturePosition"), 0);
    glUniform1i(lighting.uniformLocation("textureNormal"), 1);
    glUniform1i(lighting.uniformLocation("textureAlbedo"), 2);
    glUniform1i(lighting.uniformLocation("textureLight"), 3);
    glUniform1i(lighting.uniformLocation("textureSSAO"), 4);
    glUniform3f(lighting.uniformLocation("lightAmbient"), 1.0f, 1.0f, 1.0f);
    glUniform3f(lighting.uniformLocation("lightDiffuse"), 1.0f, 1.0f, 1.0f);
    glUniform3f(lighting.uniformLocation("lightSpecular"), 1.0f, 1.0f, 1.0f);
    lightPosIndex = lighting.uniformLocation("lightPos");
    viewPosIndex = lighting.uniformLocation("viewPos");
    CHECKERROR("lighting");

    Shader stencilVS("shaders/stencil.vs", GL_VERTEX_SHADER);
    Shader stencilFS("shaders/stencil.fs", GL_FRAGMENT_SHADER);
    stencil.addShader(stencilVS);
    stencil.addShader(stencilFS);
    stencil.link();
    stencil.use();
    stencilWVPIndex = stencil.uniformLocation("WVP");
}
SSAORenderer::~SSAORenderer()
{
    glDeleteTextures(1, &noiseTexture);
}
void SSAORenderer::setLight(glm::vec3 lightPos, glm::vec3 lightDir) const
{
    lighting.use();
    glUniform3f(lightPosIndex, lightPos.x, lightPos.y, lightPos.z);

    std::default_random_engine engine;
    std::normal_distribution<float> distr{0, 1};
    auto randDir = glm::normalize(glm::vec3{distr(engine), distr(engine), distr(engine)});
    auto look = glm::normalize(lightPos + lightDir);
    auto lightMat =
        glm::ortho(-80.0f, 80.0f, -80.0f, 80.0f, 50.0f, 200.0f) *
        glm::lookAt(lightPos, look, glm::vec3(0.0f, 1.0f, 0.0f));
    // glm::lookAt(lightPos, look, glm::cross(randDir, look));
    geometry.use();
    glUniformMatrix4fv(lightMatIndex, 1, false, glm::value_ptr(lightMat));
    glUniform3f(lightDirIndex, lightDir.x, lightDir.y, lightDir.z);
    shadow.use();
    glUniformMatrix4fv(shadowLightMatIndex, 1, false, glm::value_ptr(lightMat));
    CHECKERROR("setLight");
}
void SSAORenderer::render(std::shared_ptr<Node> root, glm::mat4 proj, const Camera &camera) const
{
    auto viewMat = camera.getTransMat();
    skybox.prepare(proj);
    skybox.render(viewMat, proj);
    shadowPass(root);
    geometryPass(root, viewMat, proj);
    ssaoPass(proj);
    blurPass();
    stencilPass(root, viewMat, proj);
    lightingPass(camera.center());
}
void SSAORenderer::geometryPass(std::shared_ptr<Node> root, glm::mat4 viewMat, glm::mat4 projMat) const
{
    geometry.use();
    gBuffer.bindForRender();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glActiveTexture(GL_TEXTURE4);
    glBindTexture(GL_TEXTURE_2D, shadowBuffer);
    root->draw([this, viewMat, projMat](int idx, glm::mat4 trans) {
        geometryRender(idx, trans, viewMat, projMat);
    });
    gBuffer.unbind();
}
void SSAORenderer::geometryRender(int idx, glm::mat4 modelMat, glm::mat4 viewMat, glm::mat4 projMat) const
{
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
    auto WVMat = viewMat * modelMat;
    glUniformMatrix4fv(WVIndex, 1, false, glm::value_ptr(WVMat));
    CHECKERROR("WV Error");
    auto WVPMat = projMat * WVMat;
    glUniformMatrix4fv(WVPIndex, 1, false, glm::value_ptr(WVPMat));
    CHECKERROR("WVPMat Error");

    glUniform1f(shininessIndex, meshes[idx].getFloatParam("shininess"));

    meshes[idx].draw();
    CHECKERROR("Draw Error");
}
void SSAORenderer::ssaoPass(glm::mat4 projMat) const
{
    ssao.use();
    gBuffer.bindAsTextures();
    glBindFramebuffer(GL_FRAMEBUFFER, ssaoFBO);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glActiveTexture(GL_TEXTURE4);
    glBindTexture(GL_TEXTURE_2D, noiseTexture);
    glUniformMatrix4fv(projMatIndex, 1, false, glm::value_ptr(projMat));
    CHECKERROR("projMat Error");
    quad.draw();
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}
void SSAORenderer::blurPass() const
{
    blur.use();
    glBindFramebuffer(GL_FRAMEBUFFER, blurFBO);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, ssaoColorBuffer);
    glClear(GL_COLOR_BUFFER_BIT);
    quad.draw();
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}
void SSAORenderer::shadowPass(std::shared_ptr<Node> root) const
{
    shadow.use();
    glBindFramebuffer(GL_FRAMEBUFFER, shadowFBO);
    glViewport(0, 0, shadowMapSize, shadowMapSize);
    glClear(GL_DEPTH_BUFFER_BIT);
    glCullFace(GL_FRONT);
    root->draw([this](int idx, glm::mat4 trans) {
        shadowRender(idx, trans);
    });
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, _width, _height);
    glCullFace(GL_BACK);
}
void SSAORenderer::shadowRender(int idx, glm::mat4 modelMat) const
{
    meshes[idx].bindVAO();
    CHECKERROR("BindVAO Error");

    glUniformMatrix4fv(shadowModelMatIndex, 1, false, glm::value_ptr(modelMat));
    CHECKERROR("modelMat Error");

    meshes[idx].draw();
    CHECKERROR("Draw Error");
}
void SSAORenderer::lightingPass(glm::vec3 viewPos) const
{
    lighting.use();
    gBuffer.bindAsTextures();
    glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
    glStencilFunc(GL_EQUAL, 1, 0xFF);
    glActiveTexture(GL_TEXTURE4);
    glBindTexture(GL_TEXTURE_2D, blurBuffer);
    glUniform3f(viewPosIndex, viewPos.x, viewPos.y, viewPos.z);
    CHECKERROR("viewPos Error");
    quad.draw();
    glDisable(GL_STENCIL_TEST);
}
void SSAORenderer::stencilPass(std::shared_ptr<Node> root, glm::mat4 viewMat, glm::mat4 projMat) const
{
    stencil.use();
    glDrawBuffer(GL_NONE);
    glEnable(GL_STENCIL_TEST);
    glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
    glStencilFunc(GL_ALWAYS, 1, 0xFF);
    glStencilMask(0xFF);
    glClear(GL_STENCIL_BUFFER_BIT);
    root->draw([this, viewMat, projMat](int idx, glm::mat4 trans) {
        stencilRender(idx, trans, viewMat, projMat);
    });
    glDrawBuffer(GL_BACK);
    glStencilMask(0x0);
}
void SSAORenderer::stencilRender(int idx, glm::mat4 modelMat, glm::mat4 viewMat, glm::mat4 projMat) const
{
    meshes[idx].bindVAO();
    CHECKERROR("BindVAO Error");
    auto WVPMat = projMat * viewMat * modelMat;
    glUniformMatrix4fv(stencilWVPIndex, 1, false, glm::value_ptr(WVPMat));
    CHECKERROR("WVPMat Error");

    meshes[idx].draw();
    CHECKERROR("Draw Error");
}

void SSAORenderer::makeKernel()
{
    std::default_random_engine engine;
    std::normal_distribution<float> distr{0, 0.2f};

    for (int i = 0; i != 64; ++i)
    {
        kernel[i * 3] = std::clamp(distr(engine), -1.0f, 1.0f);
        kernel[i * 3 + 1] = std::clamp(distr(engine), -1.0f, 1.0f);
        kernel[i * 3 + 2] = std::clamp(distr(engine), 0.0f, 1.0f);
    };
    CHECKERROR("makeKernel");
}
void SSAORenderer::makeNoise()
{
    std::default_random_engine engine;
    std::normal_distribution<float> distr{0, 0.5};
    std::array<glm::vec3, 16> noise;
    for (int i = 0; i != 16; ++i)
        noise[i] = glm::vec3{
            std::clamp(distr(engine), -1.0f, 1.0f),
            std::clamp(distr(engine), -1.0f, 1.0f),
            0.0};
    glGenBuffers(1, &noiseTexture);
    glGenTextures(1, &noiseTexture);
    glBindTexture(GL_TEXTURE_2D, noiseTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, 4, 4, 0, GL_RGB, GL_FLOAT, noise.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    CHECKERROR("makeNoise");
}
void SSAORenderer::makeSSAOFBO()
{
    glGenFramebuffers(1, &ssaoFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, ssaoFBO);

    glGenTextures(1, &ssaoColorBuffer);
    glBindTexture(GL_TEXTURE_2D, ssaoColorBuffer);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, _width, _height, 0, GL_RED, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, ssaoColorBuffer, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    CHECKERROR("makeSSAOFBO");
}
void SSAORenderer::makeBlurFBO()
{
    glGenFramebuffers(1, &blurFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, blurFBO);
    glGenTextures(1, &blurBuffer);
    glBindTexture(GL_TEXTURE_2D, blurBuffer);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, _width, _height, 0, GL_RED, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, blurBuffer, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    CHECKERROR("makeBlurFBO");
}
void SSAORenderer::makeShadowFBO()
{
    glGenFramebuffers(1, &shadowFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, shadowFBO);
    glGenTextures(1, &shadowBuffer);
    glBindTexture(GL_TEXTURE_2D, shadowBuffer);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, shadowMapSize, shadowMapSize, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    float borderColor[] = {1.0f, 1.0f, 1.0f, 1.0f};
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, shadowBuffer, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    CHECKERROR("makeShadowFBO");
}

Scene::Scene(const aiScene *scene, const std::string &directory) : ai_scene(scene), dir(directory)
{
    int meshCnt = scene->mNumMeshes;
    std::cout << "Load model" << std::endl;
    for (int i = 0; i != meshCnt; ++i)
        meshes.emplace_back(scene->mMeshes[i], this);
    root = make_shared<Node>(scene->mRootNode);
    root->applyTrans(
        glm::rotate(glm::identity<glm::mat4>(), glm::radians(-90.0f), glm::vec3(1.0, 0.0, 0.0)));

    // renderer = make_unique<BaselineRenderer>(meshes, true, true, true, false, "baseline_tangent.vs", "baseline_normals.fs");
    renderer = make_unique<SSAORenderer>(meshes, 1600, 900, true, false, true, false);
    renderer->setLight(glm::vec3{50.0f, -100.0f, 0.0f}, glm::vec3{-0.5f, 1.0f, 0.0f});
}
Scene::~Scene()
{
    for (auto &t : loadedTextures)
        glDeleteTextures(1, &(t.second.id));
}

void Scene::render(glm::mat4 proj, const Camera &camera) const
{
    renderer->render(root, proj, camera);
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
        std::cerr << "Texture failed to load at path: " << filename << std::endl;
        stbi_image_free(data);
    }

    return textureID;
}