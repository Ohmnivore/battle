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
		bool dropShadow;
	};

	typedef Oryol::Array<Sprite> Sprites;

	struct DropShadow {
		Sprite* sprite;
		glm::vec3 pos;
	};

	typedef Oryol::Array<DropShadow> DropShadows;

	struct BoxCollider {
		glm::vec2 pos;
		glm::vec2 size;
	};

	typedef Oryol::Array<BoxCollider> BoxColliders;

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

		Renderable(const DropShadow& dropShadow, const glm::mat3& transform) :
			type(RenderableType::SPRITE),
			texIdx(2), // TODO: fix this mess
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

	void Setup(AllWalls& walls, Sprites& sprites, DropShadows& dropShadows) {
		SortedDropShadows.SetFixedCapacity(dropShadows.Size());

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

		// Flip sprites horizontally if looking towards -Y axis
		glm::mat3 spriteFlip;
		if (cam.getDir().y < 0.0f)
			spriteFlip = glm::scale(glm::mat3(), glm::vec2(-1.0f, 1.0f));

		for (int spriteIdx = 0; spriteIdx < sprites.Size(); ++spriteIdx) {
			Sprite& sprite = sprites[spriteIdx];

			// Compute view-space position
			glm::vec4 modelPos(sprite.pos.x, sprite.pos.y, sprite.pos.z, 1.0f);
			glm::vec4 modelPosInViewSpace = cam.getTransformInverse() * modelPos;

			// Compute transform matrix
			glm::mat3 transform = glm::translate(glm::mat3(), glm::vec2(modelPosInViewSpace.x, modelPosInViewSpace.y + 24.0f)) * spriteFlip;

			bool top = sprite.pos.z >= TOP_BG_Z_POS;

			if (top) {
				// Add & in-place sort based on view space Z position
				Renderable rend(sprite, glm::vec3(modelPosInViewSpace), transform);
				auto insertPoint = std::upper_bound(Sorted.end() - numTopSprites, Sorted.end(), rend, &Renderer::RenderableDepthCompare);
				int idx = std::distance(Sorted.begin(), insertPoint);
				Sorted.Insert(idx, rend);

				numTopSprites++;
			}
			else {
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

	SortedRenderList& UpdateDropShadows(Camera& cam, DropShadows& shadows, BoxColliders& boxes, int& numFloorHeightShadows) {
		int numSorted = 0;
		numFloorHeightShadows = 0;
		SortedDropShadows.Clear();

		for (int shadowIdx = 0; shadowIdx < shadows.Size(); ++shadowIdx) {
			DropShadow& shadow = shadows[shadowIdx];

			shadow.pos.x = shadow.sprite->pos.x;
			shadow.pos.y = shadow.sprite->pos.y;

			bool secondFloor = false;

			for (int boxIdx = 0; boxIdx < boxes.Size(); ++boxIdx) {
				const BoxCollider& box = boxes[boxIdx];

				if (CollideCircleBox2D(glm::vec2(shadow.pos.x, shadow.pos.y), 12.0f, box)) {
					secondFloor = true;
					break;
				}
			}
			
			if (secondFloor) {
				shadow.pos.z = TOP_BG_Z_POS;

				// Bump sprite z coord to second floor
				// (just for show)
				if (shadow.sprite->pos.z < TOP_BG_Z_POS) {
					shadow.sprite->pos.z = TOP_BG_Z_POS;
				}
			}
			else {
				shadow.pos.z = BOT_BG_Z_POS;
			}

			// Compute scale
			float shadowScale = glm::max(0.5f, 1.0f - (shadow.sprite->pos.z - shadow.pos.z) / 200.0f);
			glm::mat3 scale = glm::scale(glm::mat3(), glm::vec2(shadowScale, shadowScale));

			// Compute view-space position
			glm::vec4 modelPos(shadow.pos.x, shadow.pos.y, shadow.pos.z, 1.0f);
			glm::vec4 modelPosInViewSpace = cam.getTransformInverse() * modelPos;

			// Compute transform matrix
			glm::mat3 transform = glm::translate(glm::mat3(), glm::vec2(modelPosInViewSpace.x, modelPosInViewSpace.y + 2.0f)) * scale;

			Renderable rend(shadow, transform);
			if (secondFloor) {
				SortedDropShadows.Insert(SortedDropShadows.Size() - numFloorHeightShadows, rend);
				numFloorHeightShadows++;
			}
			else {
				SortedDropShadows.Insert(numSorted, rend);
				numSorted++;
			}
		}

		return SortedDropShadows;
	}

protected:

	static bool RenderableDepthCompare(const Renderable& left, const Renderable& right) {
		return left.viewSpacePos.z < right.viewSpacePos.z;
	}

	// Implements https://gamedev.stackexchange.com/a/120897
	static bool CollideCircleBox2D(glm::vec2 circlePos, float circleRadius, BoxCollider box) {
		float r1x = box.pos.x - circleRadius;
		float r1y = box.pos.y;
		float r1w = box.size.x + circleRadius * 2.0f;
		float r1h = box.size.y;

		if (circlePos.x >= r1x && circlePos.x <= r1x + r1w &&
			circlePos.y >= r1y && circlePos.y <= r1y + r1h) {
			return true;
		}

		float r2x = box.pos.x;
		float r2y = box.pos.y - circleRadius;
		float r2w = box.size.x;
		float r2h = box.size.y + circleRadius * 2.0f;

		if (circlePos.x >= r2x && circlePos.x <= r2x + r2w &&
			circlePos.y >= r2y && circlePos.y <= r2y + r2h) {
			return true;
		}
		
		const glm::vec2 circleCenters[] = {
			glm::vec2(box.pos.x, box.pos.y),
			glm::vec2(box.pos.x, box.pos.y + box.size.y),
			glm::vec2(box.pos.x + box.size.x, box.pos.y + box.size.y),
			glm::vec2(box.pos.x + box.size.x, box.pos.y)
		};

		for (int circleIdx = 0; circleIdx < 4; ++circleIdx) {
			const glm::vec2& circleCenter = circleCenters[circleIdx];

			float dx = circlePos.x - circleCenter.x;
			float dy = circlePos.y - circleCenter.y;
			float dist = glm::sqrt(dx * dx + dy * dy);

			if (dist <= circleRadius)
				return true;
		}

		return false;
	}

	glm::mat3 TileMapAffine;
	glm::mat3 WallAffine[WallDirection::WALL_MAX_DIRECTION];

	float WallDot[WallDirection::WALL_MAX_DIRECTION];
	float WallShear[WallDirection::WALL_MAX_DIRECTION];
	bool WallVisible[WallDirection::WALL_MAX_DIRECTION];

	SortedRenderList Sorted;
	SortedRenderList SortedDropShadows;
};
