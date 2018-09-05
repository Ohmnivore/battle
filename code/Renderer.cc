#pragma once
#include "Core/Containers/Array.h"

#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtx/matrix_transform_2d.hpp"

#include <algorithm>

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
		glm::vec3 viewSpacePos;
		int dir;
		int img;
	};

	typedef Oryol::Array<Wall> WallsOfDir;
	struct AllWalls {
		WallsOfDir walls[WallDirection::WALL_MAX_DIRECTION];
	};

	typedef Oryol::Array<Wall*> SortedWalls;

	void SetNumWalls(AllWalls& walls) {
		int numWalls = 0;

		for (int dir = 0; dir < WALL_MAX_DIRECTION; ++dir) {
			numWalls += walls.walls[dir].Size();
		}

		Sorted.SetFixedCapacity(numWalls);
	}

	void Update(Camera& cam) {
		glm::mat3 scale = glm::scale(glm::mat3(), glm::vec2(1.0, glm::cos(-cam.Pitch)));
		glm::mat3 rotate = glm::rotate(scale, -cam.Heading);
		TileMapAffine = rotate;

		{
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
		}

		{
			glm::vec4 yAxis(0.0f, 1.0f, 0.0f, 0.0f);
			glm::vec4 xAxis(1.0f, 0.0f, 0.0f, 0.0f);
			glm::vec4 yTangent = cam.getTransformInverse() * yAxis;
			glm::vec4 xTangent = cam.getTransformInverse() * xAxis;

			WallShear[WallDirection::Y_PLUS] = xTangent.y / xTangent.x;
			WallShear[WallDirection::Y_MINUS] = WallShear[WallDirection::Y_PLUS];
			WallShear[WallDirection::X_PLUS] = yTangent.y / yTangent.x;
			WallShear[WallDirection::X_MINUS] = WallShear[WallDirection::X_PLUS];
		}

		for (int dir = 0; dir < WallDirection::WALL_MAX_DIRECTION; ++dir) {
			if (WallVisible[dir]) {
				glm::mat3 shear = glm::shearX(glm::mat3(), WallShear[dir]);
				glm::mat3 scale = glm::scale(shear, glm::vec2(glm::abs(WallDot[dir]), glm::abs(cam.getDir().z)));

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

	SortedWalls& SortWalls(Camera& cam, AllWalls& walls) {
		int numWallsSorted = 0;
		Sorted.Clear();

		for (int dir = 0; dir < WallDirection::WALL_MAX_DIRECTION; ++dir) {
			if (WallVisible[dir]) {
				for (int wallIdx = 0; wallIdx < walls.walls[dir].Size(); ++wallIdx) {
					Wall& wall = walls.walls[dir][wallIdx];

					// Compute view-space position
					glm::vec4 modelPos(wall.pos.x, wall.pos.y, wall.pos.z, 1.0f);
					glm::vec4 modelPosInViewSpace = cam.getTransformInverse() * modelPos;
					wall.viewSpacePos.x = modelPosInViewSpace.x;
					wall.viewSpacePos.y = modelPosInViewSpace.y;
					wall.viewSpacePos.z = modelPosInViewSpace.z;

					// Add & in-place sort based on view space Z position
					auto insertPoint = std::upper_bound(Sorted.begin(), Sorted.begin() + numWallsSorted, &wall, &Renderer::WallDepthCompare);
					int idx = std::distance(Sorted.begin(), insertPoint);
					Sorted.Insert(idx, &wall);

					numWallsSorted++;
				}
			}
		}

		return Sorted;
	}

	void RenderWall(Camera& cam, Wall& wall, glm::mat3& dst) {
		glm::vec4 modelPosInViewSpace(wall.viewSpacePos.x, wall.viewSpacePos.y, wall.viewSpacePos.z, 1.0f);
		glm::mat3 modelTranslate = glm::translate(glm::mat3(), glm::vec2(modelPosInViewSpace.x, modelPosInViewSpace.y));

		dst = modelTranslate * WallAffine[wall.dir];
	}

protected:

	static bool WallDepthCompare(const Wall* left, const Wall* right)
	{
		return left->viewSpacePos.z < right->viewSpacePos.z;
	}

	glm::mat3 TileMapAffine;
	glm::mat3 WallAffine[WallDirection::WALL_MAX_DIRECTION];

	float WallDot[WallDirection::WALL_MAX_DIRECTION];
	float WallShear[WallDirection::WALL_MAX_DIRECTION];
	bool WallVisible[WallDirection::WALL_MAX_DIRECTION];

	SortedWalls Sorted;
};
