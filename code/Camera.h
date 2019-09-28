#pragma once
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"


class Camera {

public:

    const glm::vec3& GetDir() const;

    const glm::vec2& GetDirXY() const;

    const glm::vec2& GetDirXYTwisted() const;

    const glm::mat4& GetTransform() const;

    const glm::mat4& GetTransformInverse() const;

    const glm::mat4& GetTransformInverseTwisted() const;

    void UpdateTransforms();

    glm::vec3 Pos;
    float Heading;
    float Pitch;

private:

    glm::vec3 Dir;
    glm::vec2 DirXY;
    glm::vec2 DirXYTwisted;
    glm::mat4 Transform;
    glm::mat4 TransformInverse;
    glm::mat4 TransformInverseTwisted;
};
