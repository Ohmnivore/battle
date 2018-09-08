#pragma once
#include "Core/Containers/Array.h"
#include "Core/String/String.h"

#include "Renderer.cc"

using namespace Oryol;

class MapFile {

public:

	void Load(Renderer::AllWalls& walls, Renderer::Sprites& sprites, Renderer::DropShadows& dropShadows, String& str) {
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
			ProcessLine(walls, sprites, dropShadows, lines[i]);
		}
	}

protected:

	void ProcessLine(Renderer::AllWalls& walls, Renderer::Sprites& sprites, Renderer::DropShadows& dropShadows, char* line) {
		char* word;
		char* delims = "\t ";
		word = strtok(line, delims);

		if (strcmp(word, "wall") == 0) {
			word = strtok(NULL, delims);
			int valuesIdx = 0;
			int values[4];

			while (word != NULL) {
				values[valuesIdx] = static_cast<int>(strtol(word, NULL, 10));
				valuesIdx++;
				word = strtok(NULL, delims);
			}

			if (valuesIdx == 4) {
				Renderer::Wall wall;
				Renderer::WallDirection dir = static_cast<Renderer::WallDirection>(values[0]);

				wall.pos.x = static_cast<float>(values[1]) - 256.0f;
				wall.pos.y = (512.0f - static_cast<float>(values[2])) - 256.0f;
				wall.pos.z = 16.0f * MAP_AND_WALL_HEIGHT_SCALE;
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
		}
		else if (strcmp(word, "sprite") == 0) {
			word = strtok(NULL, delims);
			bool dropShadow = strcmp(word, "true") == 0;

			word = strtok(NULL, delims);

			int valuesIdx = 0;
			int values[4];

			while (word != NULL) {
				values[valuesIdx] = static_cast<int>(strtol(word, NULL, 10));
				valuesIdx++;
				word = strtok(NULL, delims);
			}

			if (valuesIdx == 4) {
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
		}
	}
};
