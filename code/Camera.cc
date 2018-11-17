#include "Camera.h"

#include "glm/gtx/euler_angles.hpp"


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

    glm::mat4 camHeading = glm::eulerAngleZ(Heading);
    glm::mat4 camPitch = glm::eulerAngleX(Pitch);
    glm::mat4 camOrientation = camHeading * camPitch;
    Transform = glm::translate(camOrientation, Pos);

    glm::mat4 camOrientationInverse = glm::mat4(glm::inverse(glm::mat3(camOrientation))); // mat3 inverse is much faster than mat4
    TransformInverse = glm::translate(camOrientationInverse, -Pos);

    glm::mat4 camHeadingTwisted = glm::eulerAngleZ(headingTwisted);
    glm::mat4 camOrientationTwisted = camHeadingTwisted * camPitch;
    glm::mat4 camOrientationInverseTwisted = glm::mat4(glm::inverse(glm::mat3(camOrientationTwisted)));
    TransformInverseTwisted = glm::translate(camOrientationInverseTwisted, -Pos);

    DirXY.x = glm::cos(Heading);
    DirXY.y = glm::sin(Heading);

    DirXYTwisted.x = glm::cos(headingTwisted);
    DirXYTwisted.y = glm::sin(headingTwisted);

    // The direction is the transformation matrix's first column
    Dir.x = Transform[1][0];
    Dir.y = Transform[1][1];
    Dir.z = Transform[1][2];
}
