#pragma once
#include <string>
#include <vector>

#include "Core/String/String.h"

#include "Renderer.h"

using namespace Oryol;


class MapFile {

public:

    MapFile();

    void Load(Renderer::LvlData& lvl, String& str);

private:

    int NumTilemapsLoaded;

    int ReadNumber(const std::vector<std::string> words, int idx);

    void ReadNumbers(const std::vector<std::string> words, int* dest, int num, int offset);

    void ConvertUVToWorldPos(const Renderer::LvlData& lvl, int& x, int &y);

    void ProcessLine(Renderer::LvlData& lvl, const std::string& line);
};
