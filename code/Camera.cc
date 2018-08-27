#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtx/matrix_transform_2d.hpp"

class Camera {

public:

	glm::vec3 pos;
	float heading;
	float pitch;

	glm::mat4 transform;
	glm::mat4 transformInverse;

	void updateTransforms() {
		glm::mat4 camTranslate = glm::translate(glm::mat4(), pos);
		glm::mat4 camHeading = glm::rotate(camTranslate, heading, glm::vec3(0.0f, 0.0f, 1.0f));
		glm::mat4 camPitch = glm::rotate(camHeading, pitch, glm::vec3(1.0f, 0.0f, 0.0f));
		transform = camPitch;
		transformInverse = glm::inverse(transform);
	}
};
