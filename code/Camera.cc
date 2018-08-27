#pragma once
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtx/matrix_transform_2d.hpp"

class Camera {

public:

	glm::vec3 Pos;
	float Heading;
	float Pitch;

	glm::mat4 Transform;
	glm::mat4 TransformInverse;

	void updateTransforms() {
		glm::mat4 camTranslate = glm::translate(glm::mat4(), Pos);
		glm::mat4 camHeading = glm::rotate(camTranslate, Heading, glm::vec3(0.0f, 0.0f, 1.0f));
		glm::mat4 camPitch = glm::rotate(camHeading, Pitch, glm::vec3(1.0f, 0.0f, 0.0f));
		Transform = camPitch;
		TransformInverse = glm::inverse(Transform);
	}
};
