#pragma once
#include "Core/Containers/Array.h"
#include "Core/String/String.h"

#include "Renderer.cc"

using namespace Oryol;

class MapFile {

public:

	void LoadWalls(Renderer::AllWalls& walls, String& str) {
		char* cstr = strdup(str.AsCStr());

		char* line;
		char* delims = "\r\n";
		char* res = strtok(cstr, delims);
		line = strdup(res);

		// Using lines array to avoid re-entering strtok
		// Just doing this the quick&dirty way
		Array<char*> lines;

		while (line != NULL) {
			// Ignore comments
			if (line[0] != '#') {
				lines.Add(line);
			}

			// Next line
			res = strtok(NULL, delims);
			line = strdup(res);
		}

		for (int i = 0; i < lines.Size(); ++i) {
			ProcessLine(walls, lines[i]);
		}
	}

protected:

	void ProcessLine(Renderer::AllWalls& walls, char* line) {
		char* word;
		char* delims = "\t ";
		word = strtok(line, delims);

		int valuesIdx = 0;
		int values[4];

		while (word != NULL) {
			// Process this word
			values[valuesIdx] = static_cast<int>(strtol(word, NULL, 10));

			// Next word
			valuesIdx++;
			word = strtok(NULL, delims);
		}

		if (valuesIdx == 4) {
			Renderer::Wall wall;
			Renderer::WallDirection dir = static_cast<Renderer::WallDirection>(values[0]);
			wall.pos.x = static_cast<float>(values[1]);
			wall.pos.y = static_cast<float>(values[2]);
			wall.pos.z = 1.0f;
			wall.img = values[3];

			walls.walls[dir].Add(wall);
		}
	}
};
