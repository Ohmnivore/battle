#pragma once
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtx/matrix_transform_2d.hpp"

#include "Camera.cc"

class Renderer {

public:

	glm::mat3 TileMapAffine;

	void Update(Camera& cam) {
		glm::mat3 scale = glm::scale(glm::mat3(), glm::vec2(1.0, glm::cos(-cam.Pitch)));
		glm::mat3 rotate = glm::rotate(scale, -cam.Heading);
		TileMapAffine = rotate;
	}

	void RenderTileMap(Camera& cam, glm::vec3& pos, glm::mat3& dst) {
		glm::vec4 modelPos(pos.x, pos.y, pos.z, 1.0f);
		glm::vec4 modelPosInViewSpace = cam.TransformInverse * modelPos;

		glm::mat3 modelTranslate = glm::translate(glm::mat3(), glm::vec2(modelPosInViewSpace.x, modelPosInViewSpace.y));

		dst = modelTranslate * TileMapAffine;
	}
};
