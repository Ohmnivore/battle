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
	void DrawRenderable(Renderer::Renderable& rend);

    DrawState MainDrawState;
	MainShader::gl vsGLParams;
	MainShader::gba vsGBAParams;
	glm::mat4 ViewProj;

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

void BattleApp::DrawRenderable(Renderer::Renderable& rend) {
	// Workaround for https://github.com/floooh/oryol/issues/308
	// (It's really a mat3 but we pass it as a mat4)
	this->vsGBAParams.model = glm::mat4(rend.transform);

	// Update parameters
	MainDrawState.Mesh[0] = Meshes[rend.type];
	MainDrawState.FSTexture[MainShader::tex] = Res.Tex[rend.texIdx];
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
			for (int dir = 0; dir < Renderer::WALL_MAX_DIRECTION; ++dir) {
				for (int idx = 0; idx < Res.walls.walls[dir].Size(); ++idx) {
					Renderer::Wall& wall = Res.walls.walls[dir][idx];
					wall.img += Resources::TextureAsset::WALLS_BASE;
				}
			}

			for (int idx = 0; idx < Res.sprites.Size(); ++idx) {
				Renderer::Sprite& sprite = Res.sprites[idx];
				sprite.img += Resources::TextureAsset::SPRITES_BASE;
			}

			Renderer.SetNumWalls(Res.walls, Res.sprites);
			RendererIsSetup = true;
		}

		quit = Controls.UpdateCam(Cam);

		// Update parameters
		this->vsGLParams.viewProj = ViewProj;

		Cam.UpdateTransforms();
		Renderer.Update(Cam);

		this->DrawTilemap(Res.Tex[Resources::BG3], glm::vec3(0.0f, 0.0f, BOT_BG_Z_POS));

		int numTopSprites;
		Renderer::SortedRenderList& sorted = Renderer.Sort(Cam, Res.walls, Res.sprites, numTopSprites);
		for (int rendIdx = 0; rendIdx < sorted.Size() - numTopSprites; ++rendIdx) {
			this->DrawRenderable(sorted[rendIdx]);
		}

		this->DrawTilemap(Res.Tex[Resources::BG2], glm::vec3(0.0f, 0.0f, TOP_BG_Z_POS));

		for (int rendIdx = sorted.Size() - numTopSprites; rendIdx < sorted.Size(); ++rendIdx) {
			this->DrawRenderable(sorted[rendIdx]);
		}
	}

    Gfx::EndPass();
    Gfx::CommitFrame();
    
    // continue running or quit?
    return (quit || Gfx::QuitRequested()) ? AppState::Cleanup : AppState::Running;
}

AppState::Code BattleApp::OnInit() {
    // Rendering system
    auto gfxSetup = GfxSetup::Window(480, 320, "Battle"); // x2 GBA native resolution
	gfxSetup.SampleCount = 0;
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
