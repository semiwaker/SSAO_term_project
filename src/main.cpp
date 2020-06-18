#include "GLenv.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <string>

#include "utils.h"
#include "camera.h"
#include "scene.h"

// Window
int windowWidth{1600};
int windowHeight{900};
GLFWwindow *window;

// Camera
Camera camera{
    glm::vec3{0.0, -30.0, -30.0},
    glm::normalize(glm::vec3{0.0, 0.5, 1.0}),
    glm::vec3{0.0, -1.0, 0.0},
    glm::vec3{1.0, 1.0, 1.0}};

const float keySensitive = 0.3f;
const float keyRotateSensitive = 0.01f;
const float mouseSensitive = 0.001f;

// Rendor

std::unique_ptr<Scene> scene;

void updateCamera();
void update();
void prepare();
void mainLoop();

// Input
std::map<int, TimePoint> pressTime;
bool pressing[GLFW_KEY_LAST + 1];

// Window functions
void initWindow();
static void errorCallback(int error, const char *description);
static void keyCallback(GLFWwindow *window, int key, int scanCode, int action, int mods);
static void cursorPosCallback(GLFWwindow *window, double xpos, double ypos);
static void mouseButtonCallback(GLFWwindow *window, int button, int action, int mods);
static void framebufferSizeCallback(GLFWwindow *window, int width, int height);

// OpenGL Debug
void GLAPIENTRY
messageCallback(GLenum, GLenum, GLuint, GLenum, GLsizei, const GLchar *, const void *);

int main()
{
    try
    {
        initWindow();
        prepare();
        mainLoop();
    }
    catch (std::exception &e)
    {
        std::cerr << e.what() << std::endl;
    }
    return 0;
}

void updateCamera()
{
    TimePoint now = Clock::now();

    if (pressing[GLFW_KEY_A])
    {
        auto d = camera.right();
        d *= -keySensitive * duration2secs(now - pressTime.at(GLFW_KEY_A));
        camera = camera.move(d);
    }
    if (pressing[GLFW_KEY_D])
    {
        auto d = camera.right();
        d *= keySensitive * duration2secs(now - pressTime.at(GLFW_KEY_D));
        camera = camera.move(d);
    }
    if (pressing[GLFW_KEY_W])
    {
        auto d = camera.facing();
        d *= keySensitive * duration2secs(now - pressTime.at(GLFW_KEY_W));
        camera = camera.move(d);
    }
    if (pressing[GLFW_KEY_S])
    {
        auto d = camera.facing();
        d *= -keySensitive * duration2secs(now - pressTime.at(GLFW_KEY_S));
        camera = camera.move(d);
    }
    if (pressing[GLFW_KEY_SPACE])
    {
        auto d = camera.up();
        d *= keySensitive * duration2secs(now - pressTime.at(GLFW_KEY_SPACE));
        camera = camera.move(d);
    }
    if (pressing[GLFW_KEY_LEFT_SHIFT])
    {
        auto d = camera.up();
        d *= -keySensitive * duration2secs(now - pressTime.at(GLFW_KEY_LEFT_SHIFT));
        camera = camera.move(d);
    }
    if (pressing[GLFW_KEY_Q])
    {
        auto d = -keyRotateSensitive * duration2secs(now - pressTime[GLFW_KEY_Q]);
        camera = camera.rotate(static_cast<float>(d));
    }
    if (pressing[GLFW_KEY_E])
    {
        auto d = keyRotateSensitive * duration2secs(now - pressTime[GLFW_KEY_E]);
        camera = camera.rotate(static_cast<float>(d));
    }
}
void update()
{
    updateCamera();
}
void prepare()
{
    Assimp::Importer importer;
    auto ai_scene = importer.ReadFile(
        // "model/house/Old House Files/Old House 2 3D Models.3DS",
        "model/dragon/Dragon 2.5_fbx.fbx",
        // "model/Medieval tower/Medieval tower_High_.blend",
        // "model/scifi_gun.obj",
        aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_JoinIdenticalVertices | aiProcess_CalcTangentSpace);
    if (!ai_scene)
    {
        std::cerr << importer.GetErrorString() << std::endl;
        exit(1);
    }
    // scene = std::make_unique<Scene>(ai_scene, "model/house/Old House Texture");
    // scene = std::make_unique<Scene>(ai_scene, "model/dragon/textures");
    // scene = std::make_unique<Scene>(ai_scene, "model/Medieval tower/");
    scene = std::make_unique<Scene>(ai_scene, "model/dragon");
    std::cout << "Model loaded" << std::endl;
}
void mainLoop()
{
    FPSCounter fpsCounter;

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
        update();
        fpsCounter.record();
        glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
        scene->render(glm::perspective(
                          glm::radians(45.0f),
                          static_cast<float>(windowWidth) / static_cast<float>(windowHeight),
                          0.1f, 500.0f),
                      camera);
        // scene->render(glm::identity<glm::mat4>(), camera);
        glfwSwapBuffers(window);
    }
}

void initWindow()
{
    glfwSetErrorCallback(errorCallback);

    if (!glfwInit())
        exit(EXIT_FAILURE);

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_SAMPLES, 4);

    window = glfwCreateWindow(windowWidth, windowHeight, "SSAO_Term_Project", NULL, NULL);
    if (!window)
    {
        glfwTerminate();
        exit(EXIT_FAILURE);
    }

    glfwSetKeyCallback(window, keyCallback);
    glfwSetCursorPosCallback(window, cursorPosCallback);
    glfwSetMouseButtonCallback(window, mouseButtonCallback);
    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);

    glfwMakeContextCurrent(window);
    glfwSwapInterval(0);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        glfwMakeContextCurrent(NULL);
        glfwDestroyWindow(window);
        std::cerr << "Glad loading failed!" << std::endl;
        exit(1);
    }

    glViewport(0, 0, windowWidth, windowHeight);
    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_MULTISAMPLE);
    glEnable(GL_CULL_FACE);
    // glCullFace(GL_FRONT);
    glDebugMessageCallback(messageCallback, 0);
}
static void errorCallback(int error, const char *description)
{
    std::cerr << error << " " << description << std::endl;
}
static void keyCallback(GLFWwindow *window, int key, int scanCode, int action, int mods)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_RELEASE)
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    switch (action)
    {
    case GLFW_PRESS:
        pressing[key] = true;
        pressTime[key] = chrono::steady_clock::now();
        break;
    case GLFW_RELEASE:
        pressing[key] = false;
        break;
    }
}
static void cursorPosCallback(GLFWwindow *window, double xpos, double ypos)
{
    camera = camera.lookAt(
        (static_cast<float>(xpos - windowWidth / 2.0f) * mouseSensitive) * camera.right() +
        (-static_cast<float>(ypos - windowHeight / 2.0f) * mouseSensitive) * camera.up());
    glfwSetCursorPos(window, windowWidth / 2.0f, windowHeight / 2.0f);
}
static void mouseButtonCallback(GLFWwindow *window, int button, int action, int mods)
{
}
static void framebufferSizeCallback(GLFWwindow *window, int width, int height)
{
    glViewport(0, 0, width, height);
    windowWidth = width;
    windowHeight = height;
}
void GLAPIENTRY
messageCallback(GLenum source,
                GLenum type,
                GLuint id,
                GLenum severity,
                GLsizei length,
                const GLchar *message,
                const void *userParam)
{
    fprintf(stderr, "GL CALLBACK: %s type = 0x%x, source 0x%x, severity = 0x%x, message = %s\n",
            (type == GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : ""),
            source, type, severity, message);
}
