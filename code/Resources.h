#pragma once
#include "Core/Containers/Array.h"
#include "Gfx/Gfx.h"

#include "Renderer.h"

using namespace Oryol;

class Resources {

public:

    static const int MAX_MAP_FILES = 4;
    const char* MapFiles[MAX_MAP_FILES] = {
        "assets:emerald_beach.map",
        "assets:holy_summit.map",
        "assets:battle_highway.map",
        "assets:tails_lab.map",
    };

    Resources();

    void Setup();

    void Discard();

    void SwitchLvl();

    bool DoneLoading();

    bool JustDoneLoading();

    Oryol::Array<Oryol::Id> Tex;
    Renderer::LvlData Lvl;

private:

    void LoadLvl(const char* mapFile);

    bool Loaded;
    bool JustLoaded;
    bool MapFileLoaded;
    int CurMapIdx;
    Oryol::ResourceLabel Label;
};
