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

	void DrawTilemap(Id& tex, glm::vec3& pos);

    DrawState MainDrawState;
	MainShader::gl vsGLParams;
	MainShader::gba vsGBAParams;
	glm::mat4 ViewProj;

	Id TilemapMesh;
	Id WallMesh;

	Camera Cam;
	bool RendererIsSetup = false; // Hacky byproduct of our bare-bones resource system
	Renderer Renderer;
	Controls Controls;
	Resources Res;
};
OryolMain(BattleApp);


void BattleApp::DrawTilemap(Id& tex, glm::vec3& pos) {
	glm::mat3 model;
	Renderer.RenderTileMap(Cam, pos, model);
	// Workaround for https://github.com/floooh/oryol/issues/308
	// (It's really a mat3 but we pass it as a mat4)
	this->vsGBAParams.model = glm::mat4(model);

	// Update parameters
	MainDrawState.Mesh[0] = TilemapMesh;
	MainDrawState.FSTexture[MainShader::tex] = tex;
	Gfx::ApplyDrawState(MainDrawState);
	Gfx::ApplyUniformBlock(vsGLParams);
	Gfx::ApplyUniformBlock(vsGBAParams);

	// Render
	Gfx::Draw(0);
}

AppState::Code BattleApp::OnRunning() {
	bool quit = false;
	Gfx::BeginPass();

	if (Res.DoneLoading()) {
		if (!RendererIsSetup) {
			Renderer.SetNumWalls(Res.walls);
			RendererIsSetup = true;
		}

		quit = Controls.UpdateCam(Cam);

		// Update parameters
		this->vsGLParams.viewProj = ViewProj;

		Cam.UpdateTransforms();
		Renderer.Update(Cam);

		this->DrawTilemap(Res.Tex[Resources::BG3], glm::vec3(0.0f, 0.0f, 0.0f));

		Renderer::SortedWalls& sorted = Renderer.SortWalls(Cam, Res.walls);
		for (int wallIdx = 0; wallIdx < sorted.Size(); ++wallIdx) {
			Renderer::Wall& wall = *sorted[wallIdx];

			glm::mat3 model;
			Renderer.RenderWall(Cam, wall, model);
			// Workaround for https://github.com/floooh/oryol/issues/308
			// (It's really a mat3 but we pass it as a mat4)
			this->vsGBAParams.model = glm::mat4(model);

			// Update parameters
			MainDrawState.Mesh[0] = WallMesh;
			MainDrawState.FSTexture[MainShader::tex] = Res.Tex[Resources::TextureAsset::WALLS_BASE + wall.img];
			Gfx::ApplyDrawState(MainDrawState);
			Gfx::ApplyUniformBlock(vsGLParams);
			Gfx::ApplyUniformBlock(vsGBAParams);

			// Render
			Gfx::Draw(0);
		}

		this->DrawTilemap(Res.Tex[Resources::BG2], glm::vec3(0.0f, 0.0f, 32.0f));
	}

    Gfx::EndPass();
    Gfx::CommitFrame();
    
    // continue running or quit?
    return (quit || Gfx::QuitRequested()) ? AppState::Cleanup : AppState::Running;
}

AppState::Code BattleApp::OnInit() {
    // Rendering system
    auto gfxSetup = GfxSetup::Window(800, 600, "Battle");
	gfxSetup.SampleCount = 8;
    gfxSetup.DefaultPassAction = PassAction::Clear(glm::vec4(0.25f, 0.45f, 0.65f, 1.0f));
    Gfx::Setup(gfxSetup);

	// Input system
	Controls.Setup();

	// Load resources
	Res.Setup();

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

	// Setup pipeline
    Id dispShader = Gfx::CreateResource(MainShader::Setup());
    auto dispPipSetup = PipelineSetup::FromLayoutAndShader(shapeBuilderTilemap.Layout, dispShader);
    dispPipSetup.DepthStencilState.DepthWriteEnabled = false;
    dispPipSetup.RasterizerState.SampleCount = gfxSetup.SampleCount;
	dispPipSetup.BlendState.BlendEnabled = true;
	dispPipSetup.BlendState.SrcFactorRGB = BlendFactor::SrcAlpha;
	dispPipSetup.BlendState.DstFactorRGB = BlendFactor::OneMinusSrcAlpha;
    MainDrawState.Pipeline = Gfx::CreateResource(dispPipSetup);

    // Setup transform matrices
    float32 fbWidth = Gfx::DisplayAttrs().FramebufferWidth;
    float32 fbHeight = Gfx::DisplayAttrs().FramebufferHeight;
	const glm::mat4 proj = glm::ortho(-fbWidth / 4.0f, fbWidth / 4.0f, -fbHeight / 4.0f, fbHeight / 4.0f, -1000.0f, 1000.0f);
	glm::mat4 modelTform = glm::translate(glm::mat4(), glm::vec3(0.0f, 0.0f, -1.5f));
	ViewProj = proj * modelTform;
	Cam.Pos = glm::vec3(0.0f, 0.0f, 128.0f);
	Cam.Heading = 0.0f;
	Cam.Pitch = 0.0f;
    
    return App::OnInit();
}

AppState::Code BattleApp::OnCleanup() {
	Controls.Discard();
	Res.Discard();
    Gfx::Discard();

    return App::OnCleanup();
}
