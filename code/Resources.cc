#include "Assets/Gfx/TextureLoader.h"
#include "Gfx/Gfx.h"
#include "IO/IO.h"
#include "LocalFS/LocalFileSystem.h"

using namespace Oryol;

class Resources {

public:

	enum Asset {
		BG2,
		BG3,
		MAX
	};

	const char* texPaths[Asset::MAX] = {
		"assets:bg2.dds",
		"assets:bg3.dds"
	};

	Id Tex[Asset::MAX];

	void Init() {
		IOSetup ioSetup;
		ioSetup.FileSystems.Add("file", LocalFileSystem::Creator());
		ioSetup.Assigns.Add("assets:", "root:assets/");
		IO::Setup(ioSetup);

		TextureSetup texBluePrint;
		texBluePrint.Sampler.MinFilter = TextureFilterMode::Nearest;
		texBluePrint.Sampler.MagFilter = TextureFilterMode::Nearest;
		texBluePrint.Sampler.WrapU = TextureWrapMode::ClampToEdge;
		texBluePrint.Sampler.WrapV = TextureWrapMode::ClampToEdge;

		for (int idx = 0; idx < Asset::MAX; ++idx) {
			Tex[idx] = Gfx::LoadResource(TextureLoader::Create(TextureSetup::FromFile(texPaths[idx], texBluePrint)));
		}
	}

	void Discard() {

	}

	bool DoneLoading() {
		if (Loaded)
			return true;

		for (int idx = 0; idx < Asset::MAX; ++idx) {
			const auto resState = Gfx::QueryResourceInfo(Tex[idx]).State;

			if (resState != ResourceState::Valid)
				return false;
		}

		Loaded = true;
		return true;
	}

private:

	bool Loaded = false;
};
