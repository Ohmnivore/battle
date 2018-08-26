#include "Pre.h"
#include "Core/Main.h"
#include "Gfx/Gfx.h"
#include "Assets/Gfx/ShapeBuilder.h"
#include "Assets/Gfx/TextureLoader.h"
#include "Input/Input.h"
#include "IO/IO.h"
#include "LocalFS/LocalFileSystem.h"
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtx/matrix_transform_2d.hpp"
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
	glm::mat4 view;
	glm::mat4 cam;
	glm::vec3 camPos;
	glm::vec2 camAngles;

	Id texBG2;
	Id texBG3;
};
OryolMain(BattleApp);

void BattleApp::DrawTilemap(Id& tex, glm::vec3& pos) {
	glm::vec4 modelPos(pos.x, pos.y, pos.z, 1.0f);
	glm::vec4 modelPosInViewSpace = this->view * modelPos;

	glm::mat3 modelTranslate = glm::translate(glm::mat3(), glm::vec2(modelPosInViewSpace.x, modelPosInViewSpace.y));
	glm::mat3 modelScale = glm::scale(modelTranslate, glm::vec2(1.0, glm::cos(-this->camAngles.x)));
	glm::mat3 modelRotate = glm::rotate(modelScale, -this->camAngles.y);
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

	const auto resState2 = Gfx::QueryResourceInfo(this->texBG2).State;
	const auto resState3 = Gfx::QueryResourceInfo(this->texBG3).State;
	if (resState2 == ResourceState::Valid && resState3 == ResourceState::Valid) {
		// Controls
		const float movePerFrame = 4.0f;
		const float rotatePerFrame = 2.0f;
		if (Input::KeyPressed(Key::Left) || Input::KeyPressed(Key::A)) {
			this->camPos.x -= movePerFrame;
		}
		if (Input::KeyPressed(Key::Right) || Input::KeyPressed(Key::D)) {
			this->camPos.x += movePerFrame;
		}
		if (Input::KeyPressed(Key::Up) || Input::KeyPressed(Key::W)) {
			this->camPos.y += movePerFrame;
		}
		if (Input::KeyPressed(Key::Down) || Input::KeyPressed(Key::S)) {
			this->camPos.y -= movePerFrame;
		}
		if (Input::KeyPressed(Key::LeftControl)) {
			this->camPos.z -= movePerFrame;
		}
		if (Input::KeyPressed(Key::Space)) {
			this->camPos.z += movePerFrame;
		}
		if (Input::KeyPressed(Key::R)) {
			this->camAngles.y -= glm::radians(rotatePerFrame);
		}
		if (Input::KeyPressed(Key::T)) {
			this->camAngles.y += glm::radians(rotatePerFrame);
		}
		if (Input::KeyPressed(Key::F)) {
			this->camAngles.x -= glm::radians(rotatePerFrame);
		}
		if (Input::KeyPressed(Key::G)) {
			this->camAngles.x += glm::radians(rotatePerFrame);
		}
		if (Input::KeyPressed(Key::Escape)) {
			quit = true;
		}

		// Constraints
		this->camPos.z = glm::max(this->camPos.z, 0.1f);
		this->camAngles.x = glm::clamp(this->camAngles.x, 0.0f, glm::radians(85.0f));

		// Update parameters
		this->vsGLParams.viewProj = viewProj;

		glm::mat4 camTranslate = glm::translate(glm::mat4(), this->camPos);
		glm::mat4 camHeading = glm::rotate(camTranslate, this->camAngles.y, glm::vec3(0.0f, 0.0f, 1.0f));
		glm::mat4 camPitch = glm::rotate(camHeading, this->camAngles.x, glm::vec3(1.0f, 0.0f, 0.0f));
		this->cam = camPitch;
		this->view = glm::inverse(this->cam);

		DrawTilemap(texBG3, glm::vec3(0.0f, 0.0f, 0.0f));
		DrawTilemap(texBG2, glm::vec3(0.0f, 0.0f, 32.0f));
	}

    Gfx::EndPass();
    Gfx::CommitFrame();
    
    // continue running or quit?
    return (quit || Gfx::QuitRequested()) ? AppState::Cleanup : AppState::Running;
}

AppState::Code
BattleApp::OnInit() {
    // Rendering system
    auto gfxSetup = GfxSetup::Window(800, 600, "Battle");
	gfxSetup.SampleCount = 8;
    gfxSetup.DefaultPassAction = PassAction::Clear(glm::vec4(0.25f, 0.45f, 0.65f, 1.0f));
    Gfx::Setup(gfxSetup);

	// Input system
	Input::Setup();

	// setup IO system
	IOSetup ioSetup;
	ioSetup.FileSystems.Add("file", LocalFileSystem::Creator());
	ioSetup.Assigns.Add("assets:", "root:assets/");
	IO::Setup(ioSetup);

	// Load textures
	TextureSetup texBluePrint;
	texBluePrint.Sampler.MinFilter = TextureFilterMode::Nearest;
	texBluePrint.Sampler.MagFilter = TextureFilterMode::Nearest;
	texBluePrint.Sampler.WrapU = TextureWrapMode::ClampToEdge;
	texBluePrint.Sampler.WrapV = TextureWrapMode::ClampToEdge;
	this->texBG2 = Gfx::LoadResource(TextureLoader::Create(TextureSetup::FromFile("assets:bg2.dds", texBluePrint)));
	this->texBG3 = Gfx::LoadResource(TextureLoader::Create(TextureSetup::FromFile("assets:bg3.dds", texBluePrint)));

    // create display rendering resources
	ShapeBuilder shapeBuilder;
    shapeBuilder.Layout
        .Clear()
        .Add(VertexAttr::Position, VertexFormat::Float3)
        .Add(VertexAttr::TexCoord0, VertexFormat::Float2);
	const glm::mat4 rot90 = glm::rotate(glm::mat4(), glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
	shapeBuilder.Transform(rot90).Plane(512.0f, 512.0f, 4);
    this->mainDrawState.Mesh[0] = Gfx::CreateResource(shapeBuilder.Build());

    Id dispShader = Gfx::CreateResource(MainShader::Setup());
    auto dispPipSetup = PipelineSetup::FromLayoutAndShader(shapeBuilder.Layout, dispShader);
    dispPipSetup.DepthStencilState.DepthWriteEnabled = true;
    dispPipSetup.DepthStencilState.DepthCmpFunc = CompareFunc::LessEqual;
    dispPipSetup.RasterizerState.SampleCount = gfxSetup.SampleCount;
	dispPipSetup.BlendState.BlendEnabled = true;
	dispPipSetup.BlendState.SrcFactorRGB = BlendFactor::SrcAlpha;
	dispPipSetup.BlendState.DstFactorRGB = BlendFactor::OneMinusSrcAlpha;
    this->mainDrawState.Pipeline = Gfx::CreateResource(dispPipSetup);

    // setup static transform matrices
    float32 fbWidth = Gfx::DisplayAttrs().FramebufferWidth;
    float32 fbHeight = Gfx::DisplayAttrs().FramebufferHeight;
	const glm::mat4 proj = glm::ortho(-fbWidth / 2.0f, fbWidth / 2.0f, -fbHeight / 2.0f, fbHeight / 2.0f, -1000.0f, 1000.0f);
	glm::mat4 modelTform = glm::translate(glm::mat4(), glm::vec3(0.0f, 0.0f, -1.5f));
	this->viewProj = proj * modelTform;
	this->camPos = glm::vec3(0.0f, 0.0f, 64.0f);
	camAngles = glm::vec2(0.0f, 0.0f);
    
    return App::OnInit();
}

AppState::Code
BattleApp::OnCleanup() {
	Input::Discard();
	IO::Discard();
    Gfx::Discard();
    return App::OnCleanup();
}
