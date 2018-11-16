#include "Camera.h"


const glm::vec3& Camera::GetDir() const {
    return Dir;
}

const glm::vec2& Camera::GetDirXY() const {
    return DirXY;
}

const glm::vec2& Camera::GetDirXYTwisted() const {
    return DirXYTwisted;
}

const glm::mat4& Camera::GetTransform() const {
    return Transform;
}

const glm::mat4& Camera::GetTransformInverse() const {
    return TransformInverse;
}

const glm::mat4& Camera::GetTransformInverseTwisted() const {
    return TransformInverseTwisted;
}


void Camera::UpdateTransforms() {
    const float twistOffset = glm::radians(-45.0f);
    const float headingTwisted = Heading + twistOffset;

    glm::mat4 camTranslate = glm::translate(glm::mat4(), Pos);
    glm::mat4 camHeading = glm::rotate(camTranslate, Heading, glm::vec3(0.0f, 0.0f, 1.0f));
    glm::mat4 camPitch = glm::rotate(camHeading, Pitch, glm::vec3(1.0f, 0.0f, 0.0f));
    Transform = camPitch;
    TransformInverse = glm::inverse(Transform);

    glm::mat4 camHeadingTwisted = glm::rotate(camTranslate, headingTwisted, glm::vec3(0.0f, 0.0f, 1.0f));
    glm::mat4 camPitchTwisted = glm::rotate(camHeadingTwisted, Pitch, glm::vec3(1.0f, 0.0f, 0.0f));
    TransformInverseTwisted = glm::inverse(camPitchTwisted);

    DirXY.x = glm::sin(Heading);
    DirXY.y = glm::cos(Heading);

    DirXYTwisted.x = glm::sin(headingTwisted);
    DirXYTwisted.y = glm::cos(headingTwisted);

    // First column
    Dir.x = Transform[1][0];
    Dir.y = Transform[1][1];
    Dir.z = Transform[1][2];
}
