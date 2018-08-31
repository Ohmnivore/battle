#include "Core/Containers/Array.h"

#include "Renderer.cc"

using namespace Oryol;

class MapFile {

public:

	Array<glm::vec2> walls[Renderer::WallDirection::WALL_MAX_DIRECTION];

	void LoadFile() {

	}
};
