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
Camera camera;

const float keySensitive = 0.01f;
const float mouseSensitive = 0.001f;

// Rendor

Scene scene;

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

int main()
{
    initWindow();
    prepare();
    mainLoop();
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
        d *= keySensitive * duration2secs(now - pressTime.at(GLFW_KEY_S));
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
        d *= keySensitive * duration2secs(now - pressTime.at(GLFW_KEY_LEFT_SHIFT));
        camera = camera.move(d);
    }
    if (pressing[GLFW_KEY_Q])
    {
        auto d = -keySensitive * duration2secs(now - pressTime[GLFW_KEY_Q]);
        camera = camera.rotate(static_cast<float>(d));
    }
    if (pressing[GLFW_KEY_E])
    {
        auto d = keySensitive * duration2secs(now - pressTime[GLFW_KEY_E]);
        camera = camera.rotate(static_cast<float>(d));
    }
}
void update()
{
    updateCamera();
}
void prepare()
{
}
void mainLoop()
{
    FPSCounter fpsCounter;

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
        update();
        fpsCounter.record();
        scene.render();
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

    window = glfwCreateWindow(windowWidth, windowHeight, "SSAO_Term_Project", NULL, NULL);
    if (!window)
    {
        glfwTerminate();
        exit(EXIT_FAILURE);
    }

    glfwSetKeyCallback(window, keyCallback);
    glfwSetCursorPosCallback(window, cursorPosCallback);
    glfwSetMouseButtonCallback(window, mouseButtonCallback);

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);

    glfwMakeContextCurrent(window);
    glfwSwapInterval(0);
}
static void errorCallback(int error, const char *description)
{
    std::cerr << error << " " << description << std::endl;
}
static void keyCallback(GLFWwindow *window, int key, int scanCode, int action, int mods)
{
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