#pragma once
#include "Pre.h"

#include "glm/mat4x4.hpp"

#include "Core/Main.h"
#include "Gfx/Gfx.h"

#include "Camera.h"
#include "Controls.h"
#include "Renderer.h"
#include "Resources.h"
#include "shaders.h"

using namespace Oryol;


class BattleApp : public App {

public:

    AppState::Code OnRunning();

    AppState::Code OnInit();

    AppState::Code OnCleanup();

private:

    void DrawTilemap(Renderer::Tilemap& tilemap);

    void DrawRenderable(Renderer::Renderable& rend);

    Id MainRenderPass;
    DrawState MainDrawState; // Renders to texture at native GBA resolution
    MainShader::gl vsGLParams;
    MainShader::gba vsGBAParams;
    glm::mat4 ViewProj;

    DrawState ScreenQuadDrawState; // Displays render texture to screen, upscaled

    Id UnitMesh;

    Camera Cam;
    Renderer Renderer;
    Controls Controls;
    Resources Res;
};

OryolMain(BattleApp);
