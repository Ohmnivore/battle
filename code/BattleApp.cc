#include "Pre.h"

#include "Assets/Gfx/ShapeBuilder.h"
#include "Core/Main.h"
#include "Gfx/Gfx.h"

#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtx/matrix_transform_2d.hpp"

#include "Camera.cc"
#include "Controls.cc"
#include "Renderer.cc"
#include "Resources.cc"
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

	Id TilemapMesh;
	Id WallMesh;
	Id SpriteMesh;
	Id Meshes[Renderer::RENDERABLE_TYPE_MAX];

	Camera Cam;
	bool RendererIsSetup = false; // Hacky byproduct of our bare-bones resource system
	Renderer Renderer;
	Controls Controls;
	Resources Res;
};
OryolMain(BattleApp);


void BattleApp::DrawTilemap(Renderer::Tilemap& tilemap) {
	glm::mat3 model;
	Renderer.RenderTileMap(Cam, tilemap.pos, model);
	// Workaround for https://github.com/floooh/oryol/issues/308
	// (It's really a mat3 but we pass it as a mat4)
	this->vsGBAParams.model = glm::mat4(model);

	// Update parameters
	MainDrawState.Mesh[0] = TilemapMesh;
	MainDrawState.FSTexture[MainShader::tex] = Res.tex[tilemap.texIdx];
	Gfx::ApplyDrawState(MainDrawState);
	Gfx::ApplyUniformBlock(vsGLParams);
	Gfx::ApplyUniformBlock(vsGBAParams);

	// Render
	Gfx::Draw(0);
}

void BattleApp::DrawRenderable(Renderer::Renderable& rend) {
	// Workaround for https://github.com/floooh/oryol/issues/308
	// (It's really a mat3 but we pass it as a mat4)
	this->vsGBAParams.model = glm::mat4(rend.transform);

	// Update parameters
	MainDrawState.Mesh[0] = Meshes[rend.type];
	MainDrawState.FSTexture[MainShader::tex] = Res.tex[rend.texIdx];
	Gfx::ApplyDrawState(MainDrawState);
	Gfx::ApplyUniformBlock(vsGLParams);
	Gfx::ApplyUniformBlock(vsGBAParams);

	// Render
	Gfx::Draw(0);
}

AppState::Code BattleApp::OnRunning() {
	bool quit = false;
	Oryol::PassAction action = PassAction::Clear(glm::vec4(Res.lvl.bgColor[0], Res.lvl.bgColor[1], Res.lvl.bgColor[2], 255.0f) / 255.0f);
	Gfx::BeginPass(MainRenderPass, action);

	if (Res.DoneLoading()) {
		if (!RendererIsSetup) {
			Renderer.Setup(Res.lvl);
			RendererIsSetup = true;
		}

		quit = Controls.Update(Cam, Res.lvl.sprites);

		// Update parameters
		this->vsGLParams.viewProj = ViewProj;

		Cam.UpdateTransforms();
		Renderer.Update(Cam, Res.lvl);

		// Draw
		this->DrawTilemap(Res.lvl.tilemaps[Renderer::TILEMAP_BOTTOM]);

		int numFloorHeightShadows;
		Renderer::SortedRenderList& sortedDropShadows = Renderer.UpdateDropShadows(Cam, Res.lvl, numFloorHeightShadows);
		for (int rendIdx = 0; rendIdx < sortedDropShadows.Size() - numFloorHeightShadows; ++rendIdx) {
			this->DrawRenderable(sortedDropShadows[rendIdx]);
		}

		int numTopSprites;
		Renderer::SortedRenderList& sorted = Renderer.Sort(Cam, Res.lvl, numTopSprites);
		for (int rendIdx = 0; rendIdx < sorted.Size() - numTopSprites; ++rendIdx) {
			this->DrawRenderable(sorted[rendIdx]);
		}

		this->DrawTilemap(Res.lvl.tilemaps[Renderer::TILEMAP_TOP]);

		for (int rendIdx = sortedDropShadows.Size() - numFloorHeightShadows; rendIdx < sortedDropShadows.Size(); ++rendIdx) {
			this->DrawRenderable(sortedDropShadows[rendIdx]);
		}

		for (int rendIdx = sorted.Size() - numTopSprites; rendIdx < sorted.Size(); ++rendIdx) {
			this->DrawRenderable(sorted[rendIdx]);
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
    return (quit || Gfx::QuitRequested()) ? AppState::Cleanup : AppState::Running;
}

AppState::Code BattleApp::OnInit() {
    // Rendering system
    auto gfxSetup = GfxSetup::Window(SCREEN_WIDTH * 2.0f, SCREEN_HEIGHT * 2.0f, "Battle"); // x2 GBA native resolution
	gfxSetup.SampleCount = 0;
    gfxSetup.DefaultPassAction = PassAction::Clear();
    Gfx::Setup(gfxSetup);

	// Input system
	Controls.Setup();

	// Load resources
	Res.Setup("assets:emerald_beach.map");

    // Create tilemap mesh
	ShapeBuilder shapeBuilderTilemap;
	shapeBuilderTilemap.Layout
        .Clear()
        .Add(VertexAttr::Position, VertexFormat::Float3)
        .Add(VertexAttr::TexCoord0, VertexFormat::Float2);
	const glm::mat4 rot90 = glm::rotate(glm::mat4(), glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
	shapeBuilderTilemap.Transform(rot90).Plane(512.0f, 512.0f, 4);
    TilemapMesh = Gfx::CreateResource(shapeBuilderTilemap.Build());

	// Create wall mesh
	ShapeBuilder shapeBuilderWall;
	shapeBuilderWall.Layout
		.Clear()
		.Add(VertexAttr::Position, VertexFormat::Float3)
		.Add(VertexAttr::TexCoord0, VertexFormat::Float2);
	shapeBuilderWall.Transform(rot90).Plane(16.0f, 32.0f, 4);
	WallMesh = Gfx::CreateResource(shapeBuilderWall.Build());

	// Create sprite mesh
	ShapeBuilder shapeBuilderSprite;
	shapeBuilderSprite.Layout
		.Clear()
		.Add(VertexAttr::Position, VertexFormat::Float3)
		.Add(VertexAttr::TexCoord0, VertexFormat::Float2);
	shapeBuilderSprite.Transform(rot90).Plane(48.0f, 48.0f, 4);
	SpriteMesh = Gfx::CreateResource(shapeBuilderSprite.Build());

	Meshes[Renderer::RenderableType::WALL] = WallMesh;
	Meshes[Renderer::RenderableType::SPRITE] = SpriteMesh;

	// Create an offscreen render pass object with a single color attachment
	auto rtSetup = TextureSetup::RenderTarget2D(SCREEN_WIDTH, SCREEN_HEIGHT, PixelFormat::RGBA8, PixelFormat::None);
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
    auto mainPipSetup = PipelineSetup::FromLayoutAndShader(shapeBuilderTilemap.Layout, mainShader);
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
