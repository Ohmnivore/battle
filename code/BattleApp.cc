#include "Pre.h"

#include "Assets/Gfx/ShapeBuilder.h"
#include "Core/Main.h"
#include "Gfx/Gfx.h"

#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtx/matrix_transform_2d.hpp"

#include "Camera.cc"
#include "Controls.cc"
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

    DrawState mainDrawState;
	MainShader::gl vsGLParams;
	MainShader::gba vsGBAParams;

	glm::mat4 viewProj;
	Camera cam;

	Controls controls;
	Resources res;
};
OryolMain(BattleApp);


void BattleApp::DrawTilemap(Id& tex, glm::vec3& pos) {
	glm::vec4 modelPos(pos.x, pos.y, pos.z, 1.0f);
	glm::vec4 modelPosInViewSpace = cam.transformInverse * modelPos;

	glm::mat3 modelTranslate = glm::translate(glm::mat3(), glm::vec2(modelPosInViewSpace.x, modelPosInViewSpace.y));
	glm::mat3 modelScale = glm::scale(modelTranslate, glm::vec2(1.0, glm::cos(-cam.pitch)));
	glm::mat3 modelRotate = glm::rotate(modelScale, -cam.heading);
	this->vsGBAParams.model = glm::mat4(modelRotate);

	// Update parameters
	this->mainDrawState.FSTexture[MainShader::tex] = tex;
	Gfx::ApplyDrawState(this->mainDrawState);
	Gfx::ApplyUniformBlock(this->vsGLParams);
	Gfx::ApplyUniformBlock(this->vsGBAParams);

	// Render
	Gfx::Draw(0);
}

AppState::Code BattleApp::OnRunning() {
	bool quit = false;
	Gfx::BeginPass();

	if (res.DoneLoading()) {
		quit = controls.UpdateCam(cam);

		// Update parameters
		this->vsGLParams.viewProj = viewProj;
		cam.updateTransforms();
		this->DrawTilemap(res.Tex[Resources::BG3], glm::vec3(0.0f, 0.0f, 0.0f));
		this->DrawTilemap(res.Tex[Resources::BG2], glm::vec3(0.0f, 0.0f, 32.0f));
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
	controls.Setup();

	// Load resources
	res.Setup();

    // Create tilemap mesh
	ShapeBuilder shapeBuilder;
    shapeBuilder.Layout
        .Clear()
        .Add(VertexAttr::Position, VertexFormat::Float3)
        .Add(VertexAttr::TexCoord0, VertexFormat::Float2);
	const glm::mat4 rot90 = glm::rotate(glm::mat4(), glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
	shapeBuilder.Transform(rot90).Plane(512.0f, 512.0f, 4);
    this->mainDrawState.Mesh[0] = Gfx::CreateResource(shapeBuilder.Build());

	// Setup pipeline
    Id dispShader = Gfx::CreateResource(MainShader::Setup());
    auto dispPipSetup = PipelineSetup::FromLayoutAndShader(shapeBuilder.Layout, dispShader);
    dispPipSetup.DepthStencilState.DepthWriteEnabled = true;
    dispPipSetup.DepthStencilState.DepthCmpFunc = CompareFunc::LessEqual;
    dispPipSetup.RasterizerState.SampleCount = gfxSetup.SampleCount;
	dispPipSetup.BlendState.BlendEnabled = true;
	dispPipSetup.BlendState.SrcFactorRGB = BlendFactor::SrcAlpha;
	dispPipSetup.BlendState.DstFactorRGB = BlendFactor::OneMinusSrcAlpha;
    this->mainDrawState.Pipeline = Gfx::CreateResource(dispPipSetup);

    // Setup transform matrices
    float32 fbWidth = Gfx::DisplayAttrs().FramebufferWidth;
    float32 fbHeight = Gfx::DisplayAttrs().FramebufferHeight;
	const glm::mat4 proj = glm::ortho(-fbWidth / 4.0f, fbWidth / 4.0f, -fbHeight / 4.0f, fbHeight / 4.0f, -1000.0f, 1000.0f);
	glm::mat4 modelTform = glm::translate(glm::mat4(), glm::vec3(0.0f, 0.0f, -1.5f));
	viewProj = proj * modelTform;
	cam.pos = glm::vec3(0.0f, 0.0f, 64.0f);
	cam.heading = 0.0f;
	cam.pitch = 0.0f;
    
    return App::OnInit();
}

AppState::Code BattleApp::OnCleanup() {
	controls.Discard();
	res.Discard();
    Gfx::Discard();
    return App::OnCleanup();
}
