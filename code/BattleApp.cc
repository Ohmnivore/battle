#include "Pre.h"
#include "Core/Main.h"
#include "Gfx/Gfx.h"
#include "Assets/Gfx/ShapeBuilder.h"
#include "Assets/Gfx/TextureLoader.h"
#include "IO/IO.h"
#include "LocalFS/LocalFileSystem.h"
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "shaders.h"

using namespace Oryol;

// derived application class
class BattleApp : public App {
public:
    AppState::Code OnRunning();
    AppState::Code OnInit();
    AppState::Code OnCleanup();    
private:
    glm::mat4 computeMVP(const glm::mat4& proj, float32 rotX, float32 rotY, const glm::vec3& pos);

    DrawState mainDrawState;
    MainShader::params mainVSParams;
    glm::mat4 view;
    glm::mat4 displayProj;
    float32 angleX = 0.0f;
    float32 angleY = 0.0f;

	Id texBG2;
	Id texBG3;
};
OryolMain(BattleApp);

//------------------------------------------------------------------------------
AppState::Code
BattleApp::OnRunning() {
    
	Gfx::BeginPass();

	const auto resState = Gfx::QueryResourceInfo(this->texBG3).State;
	if (resState == ResourceState::Valid) {
		// update animated parameters
		this->angleY += 0.01f;
		this->angleX += 0.02f;
		this->mainVSParams.mvp = this->computeMVP(this->displayProj, -this->angleX * 0.25f, this->angleY * 0.25f, glm::vec3(0.0f, 0.0f, -1.5f));

		this->mainDrawState.FSTexture[MainShader::tex] = this->texBG3;

		Gfx::ApplyDrawState(this->mainDrawState);
		Gfx::ApplyUniformBlock(this->mainVSParams);
		Gfx::Draw(0);
	}

    Gfx::EndPass();
    Gfx::CommitFrame();
    
    // continue running or quit?
    return Gfx::QuitRequested() ? AppState::Cleanup : AppState::Running;
}

//------------------------------------------------------------------------------
AppState::Code
BattleApp::OnInit() {
    // setup rendering system
    auto gfxSetup = GfxSetup::Window(800, 600, "Battle");
	gfxSetup.SampleCount = 8;
    gfxSetup.DefaultPassAction = PassAction::Clear(glm::vec4(0.25f, 0.45f, 0.65f, 1.0f));
    Gfx::Setup(gfxSetup);

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
    shapeBuilder.Sphere(0.5f, 72.0f, 40.0f);
    this->mainDrawState.Mesh[0] = Gfx::CreateResource(shapeBuilder.Build());
    Id dispShader = Gfx::CreateResource(MainShader::Setup());
    auto dispPipSetup = PipelineSetup::FromLayoutAndShader(shapeBuilder.Layout, dispShader);
    dispPipSetup.DepthStencilState.DepthWriteEnabled = true;
    dispPipSetup.DepthStencilState.DepthCmpFunc = CompareFunc::LessEqual;
    dispPipSetup.RasterizerState.SampleCount = gfxSetup.SampleCount;
    this->mainDrawState.Pipeline = Gfx::CreateResource(dispPipSetup);

    // setup static transform matrices
    float32 fbWidth = Gfx::DisplayAttrs().FramebufferWidth;
    float32 fbHeight = Gfx::DisplayAttrs().FramebufferHeight;
    this->displayProj = glm::perspectiveFov(glm::radians(45.0f), fbWidth, fbHeight, 0.01f, 100.0f);
    this->view = glm::mat4();
    
    return App::OnInit();
}

//------------------------------------------------------------------------------
AppState::Code
BattleApp::OnCleanup() {
    Gfx::Discard();
	IO::Discard();
    return App::OnCleanup();
}

//------------------------------------------------------------------------------
glm::mat4
BattleApp::computeMVP(const glm::mat4& proj, float32 rotX, float32 rotY, const glm::vec3& pos) {
    glm::mat4 modelTform = glm::translate(glm::mat4(), pos);
    modelTform = glm::rotate(modelTform, rotX, glm::vec3(1.0f, 0.0f, 0.0f));
    modelTform = glm::rotate(modelTform, rotY, glm::vec3(0.0f, 1.0f, 0.0f));
    return proj * this->view * modelTform;
}
