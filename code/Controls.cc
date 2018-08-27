#pragma once
#include "Input/Input.h"

#include "Camera.cc"

using namespace Oryol;

class Controls {

public:

	void Setup() {
		Input::Setup();
	}

	void Discard() {
		Input::Discard();
	}

	bool UpdateCam(Camera& cam) {
		const float movePerFrame = 4.0f;
		const float rotatePerFrame = 2.0f;
		if (Input::KeyPressed(Key::Left) || Input::KeyPressed(Key::A)) {
			cam.Pos.x -= movePerFrame;
		}
		if (Input::KeyPressed(Key::Right) || Input::KeyPressed(Key::D)) {
			cam.Pos.x += movePerFrame;
		}
		if (Input::KeyPressed(Key::Up) || Input::KeyPressed(Key::W)) {
			cam.Pos.y += movePerFrame;
		}
		if (Input::KeyPressed(Key::Down) || Input::KeyPressed(Key::S)) {
			cam.Pos.y -= movePerFrame;
		}
		if (Input::KeyPressed(Key::LeftControl)) {
			cam.Pos.z -= movePerFrame;
		}
		if (Input::KeyPressed(Key::Space)) {
			cam.Pos.z += movePerFrame;
		}
		if (Input::KeyPressed(Key::R)) {
			cam.Heading -= glm::radians(rotatePerFrame);
		}
		if (Input::KeyPressed(Key::T)) {
			cam.Heading += glm::radians(rotatePerFrame);
		}
		if (Input::KeyPressed(Key::F)) {
			cam.Pitch -= glm::radians(rotatePerFrame);
		}
		if (Input::KeyPressed(Key::G)) {
			cam.Pitch += glm::radians(rotatePerFrame);
		}
		if (Input::KeyPressed(Key::Escape)) {
			return true;
		}

		// Constraints
		cam.Pos.z = glm::max(cam.Pos.z, 0.1f);
		cam.Pitch = glm::clamp(cam.Pitch, 0.0f, glm::radians(85.0f));

		return false;
	}
};
