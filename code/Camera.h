#pragma once
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"


class Camera {

public:

    const glm::vec3& GetDir();

    const glm::vec2& GetDirXY();

    const glm::mat4& GetTransform();

    const glm::mat4& GetTransformInverse();

    void UpdateTransforms();

    glm::vec3 Pos;
    float Heading;
    float Pitch;

private:

    glm::vec3 Dir;
    glm::vec2 DirXY;
    glm::mat4 Transform;
    glm::mat4 TransformInverse;
};
