#pragma once
#include "Core/Containers/Array.h"

#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtx/matrix_transform_2d.hpp"

#include "Camera.cc"

class Renderer {

public:

	enum WallDirection {
		Y_PLUS = 0,
		X_MINUS,
		Y_MINUS,
		X_PLUS,
		WALL_MAX_DIRECTION
	};

	struct Wall {
		glm::vec3 pos;
		int img;
	};

	typedef Oryol::Array<Wall> WallsOfDir;
	struct AllWalls {
		WallsOfDir walls[WallDirection::WALL_MAX_DIRECTION];
	};

	glm::mat3 TileMapAffine;
	glm::mat3 WallAffine[WallDirection::WALL_MAX_DIRECTION];
	float WallDot[WallDirection::WALL_MAX_DIRECTION];
	bool WallVisible[WallDirection::WALL_MAX_DIRECTION];

	void Update(Camera& cam) {
		glm::mat3 scale = glm::scale(glm::mat3(), glm::vec2(1.0, glm::cos(-cam.Pitch)));
		glm::mat3 rotate = glm::rotate(scale, -cam.Heading);
		TileMapAffine = rotate;

		glm::vec2 yAxis(0.0f, 1.0f);
		glm::vec2 xAxis(1.0f, 0.0f);
		float yDot = glm::dot(yAxis, cam.getDirXY());
		float xDot = glm::dot(xAxis, cam.getDirXY());

		WallVisible[WallDirection::Y_PLUS] = yDot < 0.0f;
		WallVisible[WallDirection::Y_MINUS] = yDot > 0.0f;
		WallVisible[WallDirection::X_PLUS] = xDot > 0.0f;
		WallVisible[WallDirection::X_MINUS] = xDot < 0.0f;

		WallDot[WallDirection::Y_PLUS] = -yDot;
		WallDot[WallDirection::Y_MINUS] = yDot;
		WallDot[WallDirection::X_PLUS] = -xDot;
		WallDot[WallDirection::X_MINUS] = xDot;

		for (int dir = 0; dir < WallDirection::WALL_MAX_DIRECTION; ++dir) {
			if (WallVisible[dir]) {
				glm::mat3 scale = glm::scale(glm::mat3(), glm::vec2(glm::abs(WallDot[dir]), glm::abs(cam.getDir().z)));

				WallAffine[dir] = scale;
			}
		}
	}

	void RenderTileMap(Camera& cam, glm::vec3& pos, glm::mat3& dst) {
		glm::vec4 modelPos(pos.x, pos.y, pos.z, 1.0f);
		glm::vec4 modelPosInViewSpace = cam.getTransformInverse() * modelPos;

		glm::mat3 modelTranslate = glm::translate(glm::mat3(), glm::vec2(modelPosInViewSpace.x, modelPosInViewSpace.y));

		dst = modelTranslate * TileMapAffine;
	}

	void RenderWall(WallDirection dir, Camera& cam, glm::vec3& pos, glm::mat3& dst) {
		glm::vec4 modelPos(pos.x, pos.y, pos.z, 1.0f);
		glm::vec4 modelPosInViewSpace = cam.getTransformInverse() * modelPos;

		glm::mat3 modelTranslate = glm::translate(glm::mat3(), glm::vec2(modelPosInViewSpace.x, modelPosInViewSpace.y));

		dst = modelTranslate * WallAffine[dir];
	}
};
