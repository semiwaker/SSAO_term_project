#ifndef CAMERA_H
#define CAMERA_H

#include <cassert>
#include <iostream>

#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/compatibility.hpp>

class Camera
{
public:
    Camera() = default;
    Camera(glm::vec3 cameraCenter, glm::vec3 cameraFacing, glm::vec3 cameraUp, glm::vec3 cameraScale);
    Camera(glm::vec3 cameraCenter, glm::vec3 cameraTarget, glm::vec3 viewScale = glm::vec3{1.0, 1.0, 1.0});

    glm::vec3 center() const;
    glm::vec3 facing() const;
    glm::vec3 up() const;
    glm::vec3 right() const;
    glm::vec3 scale() const;

    glm::mat4 getTransMat() const;
    glm::quat getQuat() const;

    Camera move(glm::vec3 delta);
    Camera scaling(glm::vec3 factor);
    Camera lookAt(glm::vec3 target);
    Camera rotate(float rad);

    friend Camera fromQuat(glm::vec3 center, glm::quat quat, glm::vec3 scale);

private:
    glm::vec3 _center{0.0, 0.0, 0.0};
    glm::vec3 _facing{0.0, 0.0, 1.0};
    glm::vec3 _up{0.0, 1.0, 0.0};
    glm::vec3 _scale{1.0, 1.0, 1.0};
};

Camera fromQuat(glm::vec3 center, glm::quat quat, glm::vec3 scale = glm::vec3{1.0, 1.0, 1.0});

Camera cameraLerp(const Camera &A, const Camera &B, float k);

std::ostream &operator<<(std::ostream &os, const glm::vec3 &vec);
std::ostream &operator<<(std::ostream &os, const glm::mat4 &mat);
std::ostream &operator<<(std::ostream &os, const Camera &A);

#endif