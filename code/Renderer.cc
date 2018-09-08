#pragma once
#include "Core/Containers/Array.h"

#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtx/matrix_transform_2d.hpp"

#include <algorithm>

#include "Camera.cc"

const float SCREEN_WIDTH = 240.0f;
const float SCREEN_HEIGHT = 160.0f;
const float BG_COLOR[] = { 0.0f / 255.0f, 57.0f / 255.0f, 206.0f / 255.0f, 255.0f / 255.0f };

const float MAP_AND_WALL_SCALE = 2.0f;
const float MAP_AND_WALL_HEIGHT_SCALE = 0.72f;
const float BOT_BG_Z_POS = 0.0f * MAP_AND_WALL_SCALE * MAP_AND_WALL_HEIGHT_SCALE;
const float TOP_BG_Z_POS = 32.0f * MAP_AND_WALL_SCALE * MAP_AND_WALL_HEIGHT_SCALE;

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
		int dir;
		int img;
	};

	typedef Oryol::Array<Wall> WallsOfDir;
	struct AllWalls {
		WallsOfDir walls[WallDirection::WALL_MAX_DIRECTION];
	};

	struct Sprite {
		glm::vec3 pos;
		int img;
	};

	typedef Oryol::Array<Sprite> Sprites;

	enum RenderableType {
		WALL,
		SPRITE,
		RENDERABLE_TYPE_MAX
	};

	struct Renderable {

		Renderable(const Wall& wall, const glm::vec3& viewSpacePos, const glm::mat3& transform):
			type(RenderableType::WALL),
			texIdx(wall.img),
			pos(wall.pos),
			viewSpacePos(viewSpacePos),
			transform(transform)
		{
		}

		Renderable(const Sprite& sprite, const glm::vec3& viewSpacePos, const glm::mat3& transform) :
			type(RenderableType::SPRITE),
			texIdx(sprite.img),
			pos(sprite.pos),
			viewSpacePos(viewSpacePos),
			transform(transform)
		{
		}

		RenderableType type;
		int texIdx;

		glm::vec3 pos;
		glm::vec3 viewSpacePos;
		glm::mat3 transform;
	};

	typedef Oryol::Array<Renderable> SortedRenderList;

	void SetNumWalls(AllWalls& walls, Sprites& sprites) {
		int numRenderables = sprites.Size();

		for (int dir = 0; dir < WALL_MAX_DIRECTION; ++dir) {
			numRenderables += walls.walls[dir].Size();
		}

		Sorted.SetFixedCapacity(numRenderables);
	}

	void Update(Camera& cam) {
		glm::mat3 scale = glm::scale(glm::mat3(), glm::vec2(1.0, glm::cos(-cam.Pitch)) * MAP_AND_WALL_SCALE);
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
				glm::mat3 scale = glm::scale(shear, glm::vec2(glm::abs(WallDot[dir]), glm::abs(cam.getDir().z) * MAP_AND_WALL_HEIGHT_SCALE) * MAP_AND_WALL_SCALE);

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

	SortedRenderList& Sort(Camera& cam, AllWalls& walls, Sprites& sprites, int& numTopSprites) {
		int numSorted = 0;
		numTopSprites = 0;
		Sorted.Clear();

		for (int dir = 0; dir < WallDirection::WALL_MAX_DIRECTION; ++dir) {
			if (WallVisible[dir]) {
				for (int wallIdx = 0; wallIdx < walls.walls[dir].Size(); ++wallIdx) {
					Wall& wall = walls.walls[dir][wallIdx];

					// Compute view-space position
					glm::vec4 modelPos(wall.pos.x, wall.pos.y, wall.pos.z, 1.0f);
					glm::vec4 modelPosInViewSpace = cam.getTransformInverse() * modelPos;

					// Compute transform matrix
					glm::mat3 modelTranslate = glm::translate(glm::mat3(), glm::vec2(modelPosInViewSpace.x, modelPosInViewSpace.y));
					glm::mat3 transform = modelTranslate * WallAffine[wall.dir];

					// Add & in-place sort based on view space Z position
					Renderable rend(wall, glm::vec3(modelPosInViewSpace), transform);
					auto insertPoint = std::upper_bound(Sorted.begin(), Sorted.begin() + numSorted, rend, &Renderer::RenderableDepthCompare);
					int idx = std::distance(Sorted.begin(), insertPoint);
					Sorted.Insert(idx, rend);

					numSorted++;
				}
			}
		}

		for (int spriteIdx = 0; spriteIdx < sprites.Size(); ++spriteIdx) {
			Sprite& sprite = sprites[spriteIdx];

			// Compute view-space position
			glm::vec4 modelPos(sprite.pos.x, sprite.pos.y, sprite.pos.z + 24.0f, 1.0f);
			glm::vec4 modelPosInViewSpace = cam.getTransformInverse() * modelPos;

			// Compute transform matrix
			glm::mat3 transform = glm::translate(glm::mat3(), glm::vec2(modelPosInViewSpace.x, modelPosInViewSpace.y));

			bool top = sprite.pos.z >= TOP_BG_Z_POS;

			if (top)
			{
				// Add & in-place sort based on view space Z position
				Renderable rend(sprite, glm::vec3(modelPosInViewSpace), transform);
				auto insertPoint = std::upper_bound(Sorted.end() - numTopSprites, Sorted.end(), rend, &Renderer::RenderableDepthCompare);
				int idx = std::distance(Sorted.begin(), insertPoint);
				Sorted.Insert(idx, rend);

				numTopSprites++;
			}
			else
			{
				// Add & in-place sort based on view space Z position
				Renderable rend(sprite, glm::vec3(modelPosInViewSpace), transform);
				auto insertPoint = std::upper_bound(Sorted.begin(), Sorted.begin() + numSorted, rend, &Renderer::RenderableDepthCompare);
				int idx = std::distance(Sorted.begin(), insertPoint);
				Sorted.Insert(idx, rend);

				numSorted++;
			}
		}

		return Sorted;
	}

protected:

	static bool RenderableDepthCompare(const Renderable& left, const Renderable& right)
	{
		return left.viewSpacePos.z < right.viewSpacePos.z;
	}

	glm::mat3 TileMapAffine;
	glm::mat3 WallAffine[WallDirection::WALL_MAX_DIRECTION];

	float WallDot[WallDirection::WALL_MAX_DIRECTION];
	float WallShear[WallDirection::WALL_MAX_DIRECTION];
	bool WallVisible[WallDirection::WALL_MAX_DIRECTION];

	SortedRenderList Sorted;
};
