#pragma once
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"

class Camera {

public:

	glm::vec3 Pos;
	float Heading;
	float Pitch;

	const glm::vec3& getDir() {
		return Dir;
	}

	const glm::vec2& getDirXY() {
		return DirXY;
	}

	const glm::vec2& getRightDirXY() {
		return RightDirXY;
	}

	const glm::mat4& getTransform() {
		return Transform;
	}

	const glm::mat4& getTransformInverse() {
		return TransformInverse;
	}

	void UpdateTransforms() {
		glm::mat4 camTranslate = glm::translate(glm::mat4(), Pos);
		glm::mat4 camHeading = glm::rotate(camTranslate, Heading, glm::vec3(0.0f, 0.0f, 1.0f));
		glm::mat4 camPitch = glm::rotate(camHeading, Pitch, glm::vec3(1.0f, 0.0f, 0.0f));
		Transform = camPitch;
		TransformInverse = glm::inverse(Transform);

		glm::vec2 dirXY(0.0f, 1.0f);
		DirXY.x = glm::sin(Heading);
		DirXY.y = glm::cos(Heading);

		RightDirXY.x = glm::sin(Heading + glm::degrees(90.0f));
		RightDirXY.y = glm::sin(Heading + glm::degrees(90.0f));

		glm::vec4 dir(0.0f, 1.0f, 0.0f, 0.0f);
		dir = Transform * dir;
		Dir.x = dir.x;
		Dir.y = dir.y;
		Dir.z = dir.z;
	}

private:

	glm::vec3 Dir;
	glm::vec2 DirXY;
	glm::vec2 RightDirXY;
	glm::mat4 Transform;
	glm::mat4 TransformInverse;
};
