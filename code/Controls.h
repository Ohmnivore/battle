#pragma once
#include "Camera.h"
#include "Renderer.h"

using namespace Oryol;


class Controls {

public:

    enum Mode {
        WORLD,
        LOCAL,
        SPRITE,
        SPRITE_FOLLOW,
        MAX_MODES
    };

    void Setup();

    void Discard();

    bool GetShouldSwitchLvls();

    bool Update(Camera& cam, Renderer::LvlData& lvl);

private:

    static float Lerp(float start, float end, float factor);

    Mode CurMode = SPRITE_FOLLOW;
    int CurPawnIdx = 0;
    bool ShouldSwitchLvls = false;
};
