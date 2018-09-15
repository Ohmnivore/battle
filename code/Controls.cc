#pragma once
#include "Input/Input.h"

#include "glm/gtc/matrix_transform.hpp"
#include "glm/trigonometric.hpp"

#include "Camera.cc"
#include "Renderer.cc"

using namespace Oryol;

class Controls {

public:

	enum Mode {
		WORLD,
		LOCAL,
		SPRITE,
		SPRITE_FOLLOW,
		MODE_MAX
	};

	Mode CurMode = SPRITE_FOLLOW;

	void Setup() {
		Input::Setup();
	}

	void Discard() {
		Input::Discard();
	}

	bool GetShouldSwitchLvls() {
		return ShouldSwitchLvls;
	}

	bool Update(Camera& cam, Renderer::LvlData& lvl) {
		ShouldSwitchLvls = false;

		if (Input::KeyDown(Key::Z)) {
			ShouldSwitchLvls = true;
		}

		if (Input::KeyDown(Key::N1)) {
			CurMode = WORLD;
		}
		if (Input::KeyDown(Key::N2)) {
			CurMode = LOCAL;
		}
		if (Input::KeyDown(Key::N3)) {
			CurMode = SPRITE;
		}
		if (Input::KeyDown(Key::N4)) {
			CurMode = SPRITE_FOLLOW;
		}

		glm::vec3 deltaPos;
		glm::vec2 deltaRotation; // x = heading, y = pitch

		const float movePerFrame = 3.0f;
		const float rotatePerFrame = 1.0f;
		if (Input::KeyPressed(Key::Left) || Input::KeyPressed(Key::A)) {
			deltaPos.x -= movePerFrame;
		}
		if (Input::KeyPressed(Key::Right) || Input::KeyPressed(Key::D)) {
			deltaPos.x += movePerFrame;
		}
		if (Input::KeyPressed(Key::Up) || Input::KeyPressed(Key::W)) {
			deltaPos.y += movePerFrame;
		}
		if (Input::KeyPressed(Key::Down) || Input::KeyPressed(Key::S)) {
			deltaPos.y -= movePerFrame;
		}
		if (Input::KeyPressed(Key::LeftShift) || Input::KeyPressed(Key::RightShift)) {
			deltaPos.z -= movePerFrame;
		}
		if (Input::KeyPressed(Key::Space)) {
			deltaPos.z += movePerFrame;
		}
		if (Input::KeyPressed(Key::R)) {
			deltaRotation.x -= glm::radians(rotatePerFrame);
		}
		if (Input::KeyPressed(Key::T)) {
			deltaRotation.x += glm::radians(rotatePerFrame);
		}
		if (Input::KeyPressed(Key::F)) {
			deltaRotation.y -= glm::radians(rotatePerFrame);
		}
		if (Input::KeyPressed(Key::G)) {
			deltaRotation.y += glm::radians(rotatePerFrame);
		}
		if (Input::KeyPressed(Key::Escape)) {
			return true;
		}

		if (CurMode == WORLD) {
			cam.Pos.x += deltaPos.x;
			cam.Pos.y += deltaPos.y;
			cam.Pos.z += deltaPos.z;
			cam.Heading += deltaRotation.x;
			cam.Pitch += deltaRotation.y;
		}
		else if (CurMode == LOCAL) {
			glm::mat4 camHeading = glm::rotate(glm::mat4(), cam.Heading, glm::vec3(0.0f, 0.0f, 1.0f));
			glm::vec4 localDeltaPos = camHeading * glm::vec4(deltaPos.x, deltaPos.y, 0.0f, 0.0f);

			cam.Pos.x += localDeltaPos.x;
			cam.Pos.y += localDeltaPos.y;
			cam.Pos.z += deltaPos.z;
			cam.Heading += deltaRotation.x;
			cam.Pitch += deltaRotation.y;
		}
		else if (CurMode == SPRITE || CurMode == SPRITE_FOLLOW) {
			Renderer::Sprite& pawn = lvl.sprites[CurPawnIdx];
			pawn.pos.x += deltaPos.x;
			pawn.pos.y += deltaPos.y;
			pawn.pos.z += deltaPos.z;

			pawn.pos.z = glm::max(0.0f, pawn.pos.z);

			cam.Heading += deltaRotation.x;
			cam.Pitch += deltaRotation.y;

			if (Input::KeyDown(Key::V)) {
				CurPawnIdx--;
				if (CurPawnIdx < 0)
					CurPawnIdx = lvl.sprites.Size() - 1;
			}
			if (Input::KeyDown(Key::B)) {
				CurPawnIdx = (CurPawnIdx + 1) % lvl.sprites.Size();
			}
		}

		if (CurMode == SPRITE_FOLLOW) {
			glm::vec3 targetPos;
			float targetHeading;
			float targetPitch;

			Renderer::Sprite& pawn = lvl.sprites[CurPawnIdx];

			targetPos.x = 0.0f;
			targetPos.y = pawn.pos.y + lvl.camOffset.x;
			targetPos.z = lvl.camOffset.y;

			glm::vec3 pawnPos(pawn.pos.x, pawn.pos.y, pawn.pos.z + 20.0f);

			glm::vec3 dir = glm::normalize(pawnPos - cam.Pos);
			targetHeading = -glm::atan(dir.x, dir.y);
			targetPitch = glm::acos(-dir.z);

			const float factor = 0.85f;
			cam.Pos.x = lerp(cam.Pos.x, targetPos.x, factor);
			cam.Pos.y = lerp(cam.Pos.y, targetPos.y, factor);
			cam.Pos.z = lerp(cam.Pos.z, targetPos.z, factor);
			cam.Heading = lerp(cam.Heading, targetHeading, factor);
			cam.Pitch = lerp(cam.Pitch, targetPitch, factor);
		}

		// Constraints
		cam.Pos.z = glm::max(cam.Pos.z, 0.1f);
		cam.Pitch = glm::clamp(cam.Pitch, 0.0f, glm::radians(85.0f));

		return false;
	}

private:

	static float lerp(float start, float end, float factor) {
		return start * factor + (1.0f - factor) * end;
	}

	int CurPawnIdx = 0;
	bool ShouldSwitchLvls = false;
};
