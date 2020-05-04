#include "camera.h"

Camera::Camera(glm::vec3 cameraCenter, glm::vec3 cameraFacing, glm::vec3 cameraUp, glm::vec3 cameraScale)
    : _center(cameraCenter), _facing(cameraFacing), _up(cameraUp), _scale(cameraScale)
{
}
Camera::Camera(glm::vec3 cameraCenter, glm::vec3 cameraTarget, glm::vec3 viewScale)
    : _center(cameraCenter), _scale(viewScale)
{
    _facing = glm::normalize(cameraTarget - cameraCenter);
    _up = glm::cross(glm::cross(_facing, glm::vec3{0.0, 1.0, 0.0}), _facing);
}

glm::vec3 Camera::center() const
{
    return _center;
}
glm::vec3 Camera::facing() const
{
    return _facing;
}
glm::vec3 Camera::up() const
{
    return _up;
}
glm::vec3 Camera::right() const
{
    return glm::cross(_facing, _up);
}
glm::vec3 Camera::scale() const
{
    return _scale;
}

glm::mat4 Camera::getTransMat() const
{
    return glm::lookAt(_center, _center + _facing, _up) * glm::scale(glm::identity<glm::mat4>(), _scale);
}
glm::quat Camera::getQuat() const
{
    return glm::quatLookAt(-_facing, _up);
}

Camera Camera::move(glm::vec3 delta)
{
    return Camera{_center + delta, _facing, _up, _scale};
}
Camera Camera::scaling(glm::vec3 factor)
{
    return Camera{_center, _facing, _up, _scale * factor};
}
Camera Camera::lookAt(glm::vec3 target)
{
    return Camera{_center, glm::normalize(_facing + target), _up, _scale};
}
Camera Camera::rotate(float rad)
{
    return Camera{_center, _facing, glm::angleAxis(rad, _facing) * _up, _scale};
}

Camera fromQuat(glm::vec3 center, glm::quat quat, glm::vec3 scale)
{
    Camera c;
    // auto mat = glm::mat4_cast(quat);
    c._center = center;
    c._facing = c._facing * quat;
    c._up = c._up * quat;
    c._scale = scale;
    return c;
}

Camera cameraLerp(const Camera &A, const Camera &B, float k)
{
    assert(-1e-5 < k && k < 1 + 1e-5);
    auto quatA = A.getQuat();
    auto quatB = B.getQuat();
    return fromQuat(
        glm::lerp(A.center(), B.center(), k),
        glm::lerp(quatA, quatB, k),
        glm::lerp(A.scale(), B.scale(), k));
    // return Camera(glm::lerp(A.center, B.center, k), B.facing, B.up, glm::lerp(A.scale, B.scale, k));
}

std::ostream &operator<<(std::ostream &os, const glm::vec3 &vec)
{
    os << "(" << vec.x << " " << vec.y << " " << vec.z << ")";
    return os;
}
std::ostream &operator<<(std::ostream &os, const glm::mat4 &mat)
{
    using namespace std;
    os << "[" << endl;
    for (int i = 0; i != 4; ++i)
        os << mat[i] << endl;
    os << "]" << endl;
    return os;
}

std::ostream &operator<<(std::ostream &os, const Camera &A)
{
    using namespace std;
    os << "[" << endl;
    os << "Center: " << A.center() << "," << endl;
    os << "Facing: " << A.facing() << "," << endl;
    os << "Up: " << A.up() << "," << endl;
    os << "Scale: " << A.scale() << endl;
    os << "]" << endl;
    return os;
}