#pragma once
#include "Core/Containers/Array.h"
#include "Core/String/String.h"

#include "Renderer.cc"

using namespace Oryol;

class MapFile {

public:

	Array<Renderer::Wall> walls[Renderer::WallDirection::WALL_MAX_DIRECTION];

	void LoadFile(String str) {
		char* cstr = strdup(str.AsCStr());

		char* line;
		char* delims = "\r\n";
		line = strtok(cstr, delims);

		while (line != NULL) {
			// Process this line
			ProcessLine(line);

			// Next line
			line = strtok(NULL, delims);
		}
	}

protected:

	void ProcessLine(char* line) {
		char* word;
		char* delims = "\t ";
		word = strtok(line, delims);

		int valuesIdx = 0;
		int values[4];

		while (word != NULL) {
			// Process this word

			// Ignore comments
			if (word[0] == '#') {
				break;
			}

			values[valuesIdx] = static_cast<int>(strtol(word, NULL, 10));

			// Next word
			valuesIdx++;
			word = strtok(NULL, delims);
		}

		Renderer::Wall wall;
		Renderer::WallDirection dir = static_cast<Renderer::WallDirection>(values[0]);
		wall.pos.x = static_cast<float>(values[1]);
		wall.pos.y = static_cast<float>(values[2]);
		wall.pos.z = 1.0f;
		wall.img = values[3];

		walls[dir].Add(wall);
	}
};
