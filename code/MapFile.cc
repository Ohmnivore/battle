#pragma once
#include "Core/Containers/Array.h"
#include "Core/String/String.h"

#include "pystring/pystring.h"

#include <string>
#include <vector>

#include "Renderer.cc"

using namespace Oryol;

class MapFile {

public:

	void Load(
		Renderer::AllWalls& walls,
		Renderer::Sprites& sprites,
		Renderer::DropShadows& dropShadows,
		Renderer::BoxColliders& boxColliders,
		String& str)
	{
		std::string stdStr(str.AsCStr());
		std::vector<std::string> lines;

		pystring::splitlines(stdStr, lines);

		#if BATTLE_EMSCRIPTEN
		size_t numLines = lines.size();
		#else
		// Ignore last line, it's borked, not sure why
		size_t numLines = lines.size();
		if (numLines > 0)
			numLines -= 1;
		#endif

		for (size_t i = 0; i < numLines; ++i) {
			const std::string line = pystring::strip(lines[i]);

			// Ignore comments
			if (!line.empty() && line[0] != '#') {
				ProcessLine(walls, sprites, dropShadows, boxColliders, line);
			}
		}
	}

protected:

	void ProcessLine(
		Renderer::AllWalls& walls,
		Renderer::Sprites& sprites,
		Renderer::DropShadows& dropShadows,
		Renderer::BoxColliders& boxColliders,
		const std::string& line)
	{
		std::vector<std::string> words;
		pystring::split(line, words);
		std::string& type = words[0];

		if (type == "wall") {
			int values[4];
			for (int valuesIdx = 0; valuesIdx < 4; ++valuesIdx) {
				values[valuesIdx] = static_cast<int>(strtol(words[1 + valuesIdx].c_str(), NULL, 10));
			}

			Renderer::Wall wall;
			Renderer::WallDirection dir = static_cast<Renderer::WallDirection>(values[0]);

			wall.pos.x = static_cast<float>(values[1]) - 256.0f;
			wall.pos.y = (512.0f - static_cast<float>(values[2])) - 256.0f;
			wall.img = values[3] - 1; // Convert from 1-based numbering to 0-based
			wall.dir = dir;

			if (dir == Renderer::WallDirection::Y_PLUS || dir == Renderer::WallDirection::Y_MINUS) {
				wall.pos.x += 8.0f;
			}
			else {
				wall.pos.y -= 8.0f;
			}

			wall.pos *= MAP_AND_WALL_SCALE;

			walls.walls[dir].Add(wall);
		}
		else if (type == "sprite") {
			bool dropShadow = words[1] == "true";

			int values[4];
			for (int valuesIdx = 0; valuesIdx < 4; ++valuesIdx) {
				values[valuesIdx] = static_cast<int>(strtol(words[2 + valuesIdx].c_str(), NULL, 10));
			}

			Renderer::Sprite sprite;

			sprite.img = values[0];
			sprite.pos.x = static_cast<float>(values[1]) - 256.0f;
			sprite.pos.y = (512.0f - static_cast<float>(values[2])) - 256.0f;
			sprite.pos.z = static_cast<float>(values[3]);
			sprite.dropShadow = dropShadow;

			sprites.Add(sprite);

			if (dropShadow) {
				Renderer::DropShadow dropShadow;
				dropShadow.sprite = &sprites.Back();

				dropShadows.Add(dropShadow);
			}
		}
		else if (type == "box_collider") {
			int values[4];
			for (int valuesIdx = 0; valuesIdx < 4; ++valuesIdx) {
				values[valuesIdx] = static_cast<int>(strtol(words[1 + valuesIdx].c_str(), NULL, 10));
			}

			Renderer::BoxCollider box;

			box.pos.x = static_cast<float>(values[0]) - 256.0f;
			box.pos.y = (512.0f - static_cast<float>(values[1])) - 256.0f;
			box.size.x = static_cast<float>(values[2]);
			box.size.y = static_cast<float>(values[3]);

			box.pos.y -= box.size.y; // Convert from y-down to y-up

			box.pos *= MAP_AND_WALL_SCALE;
			box.size *= MAP_AND_WALL_SCALE;

			boxColliders.Add(box);
		}
	}
};
