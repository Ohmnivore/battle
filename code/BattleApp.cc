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
#include "shaders.h"

using namespace Oryol;

class BattleApp : public App {

public:

    AppState::Code OnRunning();
    AppState::Code OnInit();
    AppState::Code OnCleanup();    

private:

    DrawState mainDrawState;
    MainShader::params mainVSParams;

    glm::mat4 viewProj;
	glm::vec3 camPos;
	glm::vec3 camDir;

	Id texBG2;
	Id texBG3;
};
OryolMain(BattleApp);

AppState::Code
BattleApp::OnRunning() {
	bool quit = false;
	Gfx::BeginPass();

	const auto resState = Gfx::QueryResourceInfo(this->texBG3).State;
	if (resState == ResourceState::Valid) {
		// Controls
		const float movePerFrame = 1.0f;
		if (Input::KeyPressed(Key::Left)) {
			this->camPos.x -= movePerFrame;
		}
		if (Input::KeyPressed(Key::Right)) {
			this->camPos.x += movePerFrame;
		}
		if (Input::KeyPressed(Key::Up)) {
			this->camPos.y += movePerFrame;
		}
		if (Input::KeyPressed(Key::Down)) {
			this->camPos.y -= movePerFrame;
		}
		if (Input::KeyPressed(Key::Escape)) {
			quit = true;
		}

		// Update parameters
		this->mainVSParams.viewProj = viewProj;
		this->mainVSParams.model = glm::mat2();
		this->mainDrawState.FSTexture[MainShader::tex] = this->texBG3;

		// Render
		Gfx::ApplyDrawState(this->mainDrawState);
		Gfx::ApplyUniformBlock(this->mainVSParams);
		Gfx::Draw(0);
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
	shapeBuilder.Transform(rot90).Plane(512.0f, 512.0f, 1);
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
	this->camPos = glm::vec3(0.0f, 0.0f, 1.0f);
	this->camDir = glm::vec3(0.0f, 0.0f, -1.0f);
    
    return App::OnInit();
}

AppState::Code
BattleApp::OnCleanup() {
	Input::Discard();
	IO::Discard();
    Gfx::Discard();
    return App::OnCleanup();
}
