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
			cam.pos.x -= movePerFrame;
		}
		if (Input::KeyPressed(Key::Right) || Input::KeyPressed(Key::D)) {
			cam.pos.x += movePerFrame;
		}
		if (Input::KeyPressed(Key::Up) || Input::KeyPressed(Key::W)) {
			cam.pos.y += movePerFrame;
		}
		if (Input::KeyPressed(Key::Down) || Input::KeyPressed(Key::S)) {
			cam.pos.y -= movePerFrame;
		}
		if (Input::KeyPressed(Key::LeftControl)) {
			cam.pos.z -= movePerFrame;
		}
		if (Input::KeyPressed(Key::Space)) {
			cam.pos.z += movePerFrame;
		}
		if (Input::KeyPressed(Key::R)) {
			cam.heading -= glm::radians(rotatePerFrame);
		}
		if (Input::KeyPressed(Key::T)) {
			cam.heading += glm::radians(rotatePerFrame);
		}
		if (Input::KeyPressed(Key::F)) {
			cam.pitch -= glm::radians(rotatePerFrame);
		}
		if (Input::KeyPressed(Key::G)) {
			cam.pitch += glm::radians(rotatePerFrame);
		}
		if (Input::KeyPressed(Key::Escape)) {
			return true;
		}

		// Constraints
		cam.pos.z = glm::max(cam.pos.z, 0.1f);
		cam.pitch = glm::clamp(cam.pitch, 0.0f, glm::radians(85.0f));

		return false;
	}
};
