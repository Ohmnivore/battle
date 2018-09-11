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

	void Load(Renderer::LvlData& lvl, String& str)
	{
		NumTilemapsLoaded = 0;

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
				ProcessLine(lvl, line);
			}
		}

		// Sort tilemaps based on Z pos
		if (lvl.tilemaps[Renderer::TILEMAP_BOTTOM].pos.z > lvl.tilemaps[Renderer::TILEMAP_TOP].pos.z) {
			// Exchange places
			Renderer::Tilemap temp = lvl.tilemaps[Renderer::TILEMAP_BOTTOM];
			lvl.tilemaps[Renderer::TILEMAP_BOTTOM] = lvl.tilemaps[Renderer::TILEMAP_TOP];
			lvl.tilemaps[Renderer::TILEMAP_TOP] = temp;
		}

		// Compute floor height
		lvl.floorHeight = lvl.tilemaps[Renderer::TILEMAP_TOP].pos.z - lvl.tilemaps[Renderer::TILEMAP_BOTTOM].pos.z;

		// Compute wall height multiplier
		lvl.wallHeightMultiplier = lvl.floorHeight / lvl.wallGfxHeight;
	}

private:

	int NumTilemapsLoaded = 0;

	void ProcessLine(Renderer::LvlData& lvl, const std::string& line)
	{
		std::vector<std::string> words;
		pystring::split(line, words);
		std::string& type = words[0];

		if (type == "tilemap") {
			int values[6];
			for (int valuesIdx = 0; valuesIdx < 6; ++valuesIdx) {
				values[valuesIdx] = static_cast<int>(strtol(words[1 + valuesIdx].c_str(), NULL, 10));
			}

			Renderer::Tilemap tilemap;

			tilemap.texIdx = values[0];
			tilemap.pos.x = static_cast<float>(values[1]) - 256.0f;
			tilemap.pos.y = (512.0f - static_cast<float>(values[2])) - 256.0f;
			tilemap.pos.z = static_cast<float>(values[3]) * MAP_AND_WALL_SCALE;
			tilemap.size.x = static_cast<float>(values[4]);
			tilemap.size.y = static_cast<float>(values[5]);

			tilemap.pos.x += tilemap.size.x / 2.0f;
			tilemap.pos.y -= tilemap.size.y / 2.0f;

			lvl.tilemaps[NumTilemapsLoaded] = tilemap;
			NumTilemapsLoaded++;
		}
		else if (type == "wall") {
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

			lvl.walls.walls[dir].Add(wall);
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

			lvl.sprites.Add(sprite);

			if (dropShadow) {
				Renderer::DropShadow dropShadow;
				dropShadow.sprite = &lvl.sprites.Back();

				lvl.dropShadows.Add(dropShadow);
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

			lvl.boxColliders.Add(box);
		}
		else if (type == "opt") {
			if (words[1] == "wall_gfx_height") {
				lvl.wallGfxHeight = static_cast<float>(strtol(words[2].c_str(), NULL, 10));
				lvl.wallGfxHeight *= MAP_AND_WALL_SCALE;
			}
			else if (words[1] == "floor_sort_offset") {
				lvl.floorSortOffset = static_cast<float>(strtol(words[2].c_str(), NULL, 10));
			}
			else if (words[1] == "drop_shadow") {
				lvl.dropShadowTexIdx = static_cast<int>(strtol(words[2].c_str(), NULL, 10));
			}
			else if (words[1] == "bg_color") {
				int values[3];
				for (int valuesIdx = 0; valuesIdx < 3; ++valuesIdx) {
					values[valuesIdx] = static_cast<int>(strtol(words[2 + valuesIdx].c_str(), NULL, 10));
				}

				for (int valuesIdx = 0; valuesIdx < 3; ++valuesIdx) {
					lvl.bgColor[valuesIdx] = values[valuesIdx];
				}
			}
		}
	}
};
