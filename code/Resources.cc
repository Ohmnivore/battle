#pragma once
#include "Assets/Gfx/TextureLoader.h"
#include "Core/Containers/Array.h"
#include "Gfx/Gfx.h"
#include "IO/IO.h"
#include "HttpFS/HTTPFileSystem.h"
#include "LocalFS/LocalFileSystem.h"

#include "MapFile.cc"
#include "Renderer.cc"

using namespace Oryol;

class Resources {

public:

	Oryol::Array<Oryol::Id> tex;
	Renderer::LvlData lvl;

	void Setup(const char* mapFile) {
		IOSetup ioSetup;
		#if BATTLE_LOAD_DATA_FROM_WEB
		ioSetup.FileSystems.Add("http", HTTPFileSystem::Creator());
		ioSetup.Assigns.Add("assets:", "http://localhost:8000/assets/");
		#else
		ioSetup.FileSystems.Add("file", LocalFileSystem::Creator());
		// root: is defined by LocalFileSystem and points to our executable's directory
		ioSetup.Assigns.Add("assets:", "root:assets/");
		#endif
		IO::Setup(ioSetup);

		IO::Load(mapFile, [this](IO::LoadResult res) {
			const uint8_t* ptr = res.Data.Data();

			String str(ptr);
			MapFile mapFile;
			mapFile.Load(lvl, str);
			MapFileLoaded = true;

			TextureSetup texBluePrint;
			texBluePrint.Sampler.MinFilter = TextureFilterMode::Nearest;
			texBluePrint.Sampler.MagFilter = TextureFilterMode::Nearest;
			texBluePrint.Sampler.WrapU = TextureWrapMode::ClampToEdge;
			texBluePrint.Sampler.WrapV = TextureWrapMode::ClampToEdge;

			for (int idx = 0; idx < lvl.texPaths.Size(); ++idx) {
				tex.Add(
					Gfx::LoadResource(TextureLoader::Create(TextureSetup::FromFile(lvl.texPaths[idx].AsCStr(), texBluePrint)))
				);
			}
		});
	}

	void Discard() {

	}

	bool DoneLoading() {
		if (!MapFileLoaded)
			return false;

		if (Loaded)
			return true;

		for (int idx = 0; idx < tex.Size(); ++idx) {
			const auto resState = Gfx::QueryResourceInfo(tex[idx]).State;
			if (resState != ResourceState::Valid)
				return false;
		}

		Loaded = true;
		return true;
	}

private:

	bool Loaded = false;
	bool MapFileLoaded = false;
};
