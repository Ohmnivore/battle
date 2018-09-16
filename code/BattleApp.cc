#include "BattleApp.h"

#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtx/matrix_transform_2d.hpp"

#include "Assets/Gfx/ShapeBuilder.h"

using namespace Oryol;


void BattleApp::DrawTilemap(Renderer::Tilemap& tilemap) {
    vsGBAParams.size = Res.Lvl.texSizes[tilemap.texIdx];

    glm::mat3 model;
    Renderer.RenderTilemap(Cam, tilemap.pos, model);
    // Workaround for https://github.com/floooh/oryol/issues/308
    // (It's really a mat3 but we pass it as a mat4)
    vsGBAParams.model = glm::mat4(model);

    // Update parameters
    MainDrawState.Mesh[0] = UnitMesh;
    MainDrawState.FSTexture[MainShader::tex] = Res.Tex[tilemap.texIdx];
    Gfx::ApplyDrawState(MainDrawState);
    Gfx::ApplyUniformBlock(vsGLParams);
    Gfx::ApplyUniformBlock(vsGBAParams);

    // Render
    Gfx::Draw(0);
}

void BattleApp::DrawRenderable(Renderer::Renderable& rend) {
    vsGBAParams.size = Res.Lvl.texSizes[rend.texIdx];

    // Workaround for https://github.com/floooh/oryol/issues/308
    // (It's really a mat3 but we pass it as a mat4)
    vsGBAParams.model = glm::mat4(rend.transform);

    // Update parameters
    MainDrawState.Mesh[0] = UnitMesh;
    MainDrawState.FSTexture[MainShader::tex] = Res.Tex[rend.texIdx];
    Gfx::ApplyDrawState(MainDrawState);
    Gfx::ApplyUniformBlock(vsGLParams);
    Gfx::ApplyUniformBlock(vsGBAParams);

    // Render
    Gfx::Draw(0);
}


AppState::Code BattleApp::OnRunning() {
    bool quit = false;
    Oryol::PassAction action = PassAction::Clear(glm::vec4(Res.Lvl.bgColor[0], Res.Lvl.bgColor[1], Res.Lvl.bgColor[2], 255.0f) / 255.0f);
    Gfx::BeginPass(MainRenderPass, action);

    if (Res.DoneLoading()) {
        if (Res.JustDoneLoading()) {
            Renderer.Setup(Res.Lvl);
        }

        quit = Controls.Update(Cam, Res.Lvl);

        // Update parameters
        vsGLParams.viewProj = ViewProj;

        Cam.UpdateTransforms();
        Renderer.Update(Cam, Res.Lvl);

        // Draw
        DrawTilemap(Res.Lvl.tilemaps[Renderer::TILEMAP_BOTTOM]);

        int numFloorHeightShadows;
        Renderer::SortedRenderList& sortedDropShadows = Renderer.UpdateDropShadows(Cam, Res.Lvl, numFloorHeightShadows);
        for (int rendIdx = 0; rendIdx < sortedDropShadows.Size() - numFloorHeightShadows; ++rendIdx) {
            DrawRenderable(sortedDropShadows[rendIdx]);
        }

        int numTopSprites;
        Renderer::SortedRenderList& sorted = Renderer.Sort(Cam, Res.Lvl, numTopSprites);
        for (int rendIdx = 0; rendIdx < sorted.Size() - numTopSprites; ++rendIdx) {
            DrawRenderable(sorted[rendIdx]);
        }

        DrawTilemap(Res.Lvl.tilemaps[Renderer::TILEMAP_TOP]);

        for (int rendIdx = sortedDropShadows.Size() - numFloorHeightShadows; rendIdx < sortedDropShadows.Size(); ++rendIdx) {
            DrawRenderable(sortedDropShadows[rendIdx]);
        }

        for (int rendIdx = sorted.Size() - numTopSprites; rendIdx < sorted.Size(); ++rendIdx) {
            DrawRenderable(sorted[rendIdx]);
        }

        if (Controls.GetShouldSwitchLvls()) {
            Res.SwitchLvl();
        }
    }

    Gfx::EndPass();

    // Render GBA screen buffer to our window
    Gfx::BeginPass();
    Gfx::ApplyDrawState(ScreenQuadDrawState);
    Gfx::Draw();
    Gfx::EndPass();

    Gfx::CommitFrame();
    
    // continue running or quit?
#if BATTLE_EMSCRIPTEN
    quit = false; // Doesn't apply in browser
#endif
    return (quit || Gfx::QuitRequested()) ? AppState::Cleanup : AppState::Running;
}


AppState::Code BattleApp::OnInit() {
    // Rendering system
    auto gfxSetup = GfxSetup::Window(Renderer::SCREEN_WIDTH * 2.0f, Renderer::SCREEN_HEIGHT * 2.0f, "Battle"); // x2 GBA native resolution
    gfxSetup.SampleCount = 0;
    gfxSetup.DefaultPassAction = PassAction::Clear();
    Gfx::Setup(gfxSetup);

    // Input system
    Controls.Setup();

    // Load resources
    Res.Setup();

    // Create tilemap mesh
    ShapeBuilder shapeBuilder;
    shapeBuilder.Layout
        .Clear()
        .Add(VertexAttr::Position, VertexFormat::Float3)
        .Add(VertexAttr::TexCoord0, VertexFormat::Float2);
    const glm::mat4 rot90 = glm::rotate(glm::mat4(), glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    shapeBuilder.Transform(rot90).Plane(1.0f, 1.0f, 4);
    UnitMesh = Gfx::CreateResource(shapeBuilder.Build());

    // Create an offscreen render pass object with a single color attachment
    auto rtSetup = TextureSetup::RenderTarget2D(Renderer::SCREEN_WIDTH, Renderer::SCREEN_HEIGHT, PixelFormat::RGBA8, PixelFormat::None);
    rtSetup.Sampler.WrapU = TextureWrapMode::ClampToEdge;
    rtSetup.Sampler.WrapV = TextureWrapMode::ClampToEdge;
    rtSetup.Sampler.MagFilter = TextureFilterMode::Nearest;
    rtSetup.Sampler.MinFilter = TextureFilterMode::Nearest;
    Id rtTexture = Gfx::CreateResource(rtSetup);
    auto rpSetup = PassSetup::From(rtTexture);
    rpSetup.DefaultAction = PassAction::Clear();
    MainRenderPass = Gfx::CreateResource(rpSetup);

    // Setup pipeline for offscreen rendering
    Id mainShader = Gfx::CreateResource(MainShader::Setup());
    auto mainPipSetup = PipelineSetup::FromLayoutAndShader(shapeBuilder.Layout, mainShader);
    mainPipSetup.BlendState.ColorFormat = rtSetup.ColorFormat;
    mainPipSetup.BlendState.DepthFormat = rtSetup.DepthFormat;
    mainPipSetup.RasterizerState.SampleCount = rtSetup.SampleCount;
    mainPipSetup.DepthStencilState.DepthWriteEnabled = false;
    mainPipSetup.BlendState.BlendEnabled = true;
    mainPipSetup.BlendState.ColorWriteMask = PixelChannel::RGB;
    mainPipSetup.BlendState.SrcFactorRGB = BlendFactor::SrcAlpha;
    mainPipSetup.BlendState.DstFactorRGB = BlendFactor::OneMinusSrcAlpha;
    MainDrawState.Pipeline = Gfx::CreateResource(mainPipSetup);

    // Setup pipeline for screen quad
    ShapeBuilder shapeBuilderScreenQuad;
    shapeBuilderScreenQuad.Layout
        .Clear()
        .Add(VertexAttr::Position, VertexFormat::Float3)
        .Add(VertexAttr::TexCoord0, VertexFormat::Float2);
    shapeBuilderScreenQuad.Transform(rot90).Plane(2.0f, 2.0f, 4);
    ScreenQuadDrawState.Mesh[0] = Gfx::CreateResource(shapeBuilderScreenQuad.Build());

    Id screenQuadShader = Gfx::CreateResource(ScreenQuadShader::Setup());
    auto screenQuadPipSetup = PipelineSetup::FromLayoutAndShader(shapeBuilderScreenQuad.Layout, screenQuadShader);
    screenQuadPipSetup.RasterizerState.SampleCount = gfxSetup.SampleCount;
    ScreenQuadDrawState.Pipeline = Gfx::CreateResource(screenQuadPipSetup);
    ScreenQuadDrawState.FSTexture[ScreenQuadShader::tex] = rtTexture;

    // Setup transform matrices
    const glm::mat4 proj = glm::ortho(-rtSetup.Width / 2.0f, rtSetup.Width / 2.0f, -rtSetup.Height / 2.0f, rtSetup.Height / 2.0f, -1000.0f, 1000.0f);
    glm::mat4 modelTform = glm::translate(glm::mat4(), glm::vec3(0.0f, 0.0f, -1.5f));
    ViewProj = proj * modelTform;
    Cam.Pos = glm::vec3(0.0f, 64.0f, 0.0f);
    Cam.Heading = glm::radians(10.0f);
    Cam.Pitch = glm::radians(55.0f);
    
    return App::OnInit();
}


AppState::Code BattleApp::OnCleanup() {
    Controls.Discard();
    Res.Discard();
    Gfx::Discard();

    return App::OnCleanup();
}
