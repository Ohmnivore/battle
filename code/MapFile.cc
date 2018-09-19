#include "MapFile.h"

#include "pystring/pystring.h"

#include "Gfx/Gfx.h"

using namespace Oryol;


MapFile::MapFile() :
    NumTilemapsLoaded(0)
{
}


void MapFile::Load(Renderer::LvlData& lvl, String& str) {
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


int MapFile::ReadInt(const std::vector<std::string> words, int idx) {
    return static_cast<int>(strtol(words[idx].c_str(), NULL, 10));
}

float MapFile::ReadFloat(const std::vector<std::string> words, int idx) {
    return strtof(words[idx].c_str(), NULL);
}

void MapFile::ReadInts(const std::vector<std::string> words, int* dest, int num, int offset) {
    for (int valuesIdx = 0; valuesIdx < num; ++valuesIdx) {
        dest[valuesIdx] = ReadInt(words, offset + valuesIdx);
    }
}

void MapFile::ConvertUVToWorldPos(const Renderer::LvlData& lvl, int& x, int &y) {
    const Renderer::Tilemap& tilemap = lvl.tilemaps[Renderer::TILEMAP_TOP];
    int tilemapX = static_cast<int>(tilemap.pos.x);
    int tilemapY = static_cast<int>(tilemap.pos.y);
    int tilemapWidth = static_cast<int>(lvl.texSizes[tilemap.texIdx].x);
    int tilemapHeight = static_cast<int>(lvl.texSizes[tilemap.texIdx].y);

    x = x - tilemapWidth / 2 + tilemapX;
    y = (tilemapHeight - y) - tilemapHeight / 2 + tilemapY;
}


void MapFile::ProcessLine(Renderer::LvlData& lvl, const std::string& line) {
    std::vector<std::string> words;
    pystring::split(line, words);
    std::string& type = words[0];

    if (type == "tex") {
        lvl.texPaths.Add(Oryol::String(words[1].c_str()));

        float texWidth = static_cast<float>(ReadInt(words, 2));
        float texHeight = static_cast<float>(ReadInt(words, 3));
        lvl.texSizes.Add(glm::vec2(texWidth, texHeight));
    }
    else if (type == "tilemap") {
        const int numValues = 4;
        int values[numValues];
        ReadInts(words, values, numValues, 1);

        Renderer::Tilemap tilemap;

        tilemap.texIdx = values[0];
        tilemap.pos.x = static_cast<float>(values[1]);
        tilemap.pos.y = static_cast<float>(values[2]);
        tilemap.pos.z = static_cast<float>(values[3]);

        lvl.tilemaps[NumTilemapsLoaded] = tilemap;
        NumTilemapsLoaded++;
    }
    else if (type == "wall") {
        const int numValues = 4;
        int values[numValues];
        ReadInts(words, values, numValues, 1);

        Renderer::Wall wall;
        Renderer::WallDirection dir = static_cast<Renderer::WallDirection>(values[0]);

        ConvertUVToWorldPos(lvl, values[1], values[2]);
        wall.pos.x = static_cast<float>(values[1]);
        wall.pos.y = static_cast<float>(values[2]);
        wall.texIdx = values[3];
        wall.dir = dir;

        // SPECIAL case for wall X scale:
        // Add a copy of the texture entry with updated size
        if (words.size() > 5) {
            float xScale = ReadFloat(words, 5);

            int newTexIdx = lvl.texPaths.Size();
            Oryol::String copyPath = lvl.texPaths[wall.texIdx];
            lvl.texPaths.Add(copyPath);

            glm::vec2 size = lvl.texSizes[wall.texIdx];
            size.x *= xScale;
            lvl.texSizes.Add(size);

            wall.texIdx = newTexIdx;
        }

        float wallWidth = lvl.texSizes[wall.texIdx].x;

        if (dir == Renderer::WallDirection::Y_PLUS || dir == Renderer::WallDirection::Y_MINUS) {
            wall.pos.x += wallWidth / 2.0f;
        }
        else {
            wall.pos.y -= wallWidth / 2.0f;
        }

        wall.pos *= Renderer::MAP_AND_WALL_SCALE;

        lvl.walls.walls[dir].Add(wall);
    }
    else if (type == "sprite") {
        bool dropShadow = words[1] == "true";

        const int numValues = 4;
        int values[numValues];
        ReadInts(words, values, numValues, 2);

        Renderer::Sprite sprite;

        sprite.texIdx = values[0];
        ConvertUVToWorldPos(lvl, values[1], values[2]);
        sprite.pos.x = static_cast<float>(values[1]);
        sprite.pos.y = static_cast<float>(values[2]);
        sprite.pos.z = static_cast<float>(values[3]);
        sprite.useDropShadow = dropShadow;

        sprite.pos *= Renderer::MAP_AND_WALL_SCALE;

        lvl.sprites.Add(sprite);

        if (dropShadow) {
            Renderer::DropShadow dropShadow;
            dropShadow.sprite = &lvl.sprites.Back();

            lvl.dropShadows.Add(dropShadow);
        }
    }
    else if (type == "box_collider") {
        const int numValues = 4;
        int values[numValues];
        ReadInts(words, values, numValues, 1);

        Renderer::BoxCollider box;

        ConvertUVToWorldPos(lvl, values[0], values[1]);
        box.pos.x = static_cast<float>(values[0]);
        box.pos.y = static_cast<float>(values[1]);
        box.size.x = static_cast<float>(values[2]);
        box.size.y = static_cast<float>(values[3]);

        box.pos.y -= box.size.y; // Convert from y-down to y-up

        box.pos *= Renderer::MAP_AND_WALL_SCALE;
        box.size *= Renderer::MAP_AND_WALL_SCALE;

        lvl.boxColliders.Add(box);
    }
    else if (type == "opt") {
        if (words[1] == "wall_gfx_height") {
            lvl.wallGfxHeight = static_cast<float>(ReadInt(words, 2));
            lvl.wallGfxHeight *= Renderer::MAP_AND_WALL_SCALE;
        }
        else if (words[1] == "floor_sort_offset") {
            lvl.floorSortOffset = static_cast<float>(ReadInt(words, 2));
        }
        else if (words[1] == "drop_shadow") {
            lvl.dropShadowTexIdx = ReadInt(words, 2);
        }
        else if (words[1] == "cam_offset") {
            lvl.camOffset.x = static_cast<float>(ReadInt(words, 2));
            lvl.camOffset.y = static_cast<float>(ReadInt(words, 3));
        }
        else if (words[1] == "bg_color") {
            const int numValues = 3;
            int values[numValues];
            ReadInts(words, values, numValues, 2);

            for (int valuesIdx = 0; valuesIdx < 3; ++valuesIdx) {
                lvl.bgColor[valuesIdx] = values[valuesIdx];
            }
        }
    }
}
