#pragma once
#include "Core/Containers/Array.h"
#include "Core/String/String.h"

#include "Renderer.cc"

using namespace Oryol;

class MapFile {

public:

	void Load(Renderer::AllWalls& walls, Renderer::Sprites& sprites, String& str) {
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

		// Temp code
		Renderer::Sprite sprite1;
		sprite1.img = 0;
		sprite1.pos.x = 0.0f;
		sprite1.pos.y = 0.0f;
		sprite1.pos.z = 0.0f;
		sprites.Add(sprite1);

		Renderer::Sprite sprite2;
		sprite2.img = 1;
		sprite2.pos.x = 0.0f;
		sprite2.pos.y = 48.0f;
		sprite2.pos.z = 0.0f;
		sprites.Add(sprite2);

		Renderer::Sprite sprite3;
		sprite3.img = 2;
		sprite3.pos.x = 48.0f;
		sprite3.pos.y = 0.0f;
		sprite3.pos.z = 48.0f;
		sprites.Add(sprite3);
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

			wall.pos.x = static_cast<float>(values[1]) - 256.0f;
			wall.pos.y = (512.0f - static_cast<float>(values[2])) - 256.0f;
			wall.pos.z = 16.0f;
			wall.img = values[3] - 1; // Convert from 1-based numbering to 0-based
			wall.dir = dir;

			if (dir == Renderer::WallDirection::Y_PLUS || dir == Renderer::WallDirection::Y_MINUS) {
				wall.pos.x += 8.0f;
			}
			else {
				wall.pos.y -= 8.0f;
			}

			walls.walls[dir].Add(wall);
		}
	}
};
