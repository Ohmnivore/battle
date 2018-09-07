#pragma once
#include "Assets/Gfx/TextureLoader.h"
#include "Gfx/Gfx.h"
#include "IO/IO.h"
#include "LocalFS/LocalFileSystem.h"

#include "MapFile.cc"
#include "Renderer.cc"

using namespace Oryol;

class Resources {

public:

	enum TextureAsset {
		BG2,
		BG3,
		WALLS_BASE,
		WALLS1 = WALLS_BASE,
		WALLS2,
		WALLS3,
		WALLS4,
		WALLS5,
		SPRITES_BASE,
		CREAM = SPRITES_BASE,
		GAMMA,
		KNUCKLES,
		TEXTURE_ASSET_MAX
	};

	const char* TexPaths[TextureAsset::TEXTURE_ASSET_MAX] = {
		"assets:bg2.dds",
		"assets:bg3.dds",
		"assets:walls/1.dds",
		"assets:walls/2.dds",
		"assets:walls/3.dds",
		"assets:walls/4.dds",
		"assets:walls/5.dds",
		"assets:chars/cream.dds",
		"assets:chars/gamma.dds",
		"assets:chars/knuckles.dds",
	};
	const char* MapFilePath = "assets:walls.txt";

	Id Tex[TextureAsset::TEXTURE_ASSET_MAX];

	Renderer::AllWalls walls;
	Renderer::Sprites sprites;

	void Setup() {
		IOSetup ioSetup;
		ioSetup.FileSystems.Add("file", LocalFileSystem::Creator());
		ioSetup.Assigns.Add("assets:", "root:assets/");
		IO::Setup(ioSetup);

		TextureSetup texBluePrint;
		texBluePrint.Sampler.MinFilter = TextureFilterMode::Nearest;
		texBluePrint.Sampler.MagFilter = TextureFilterMode::Nearest;
		texBluePrint.Sampler.WrapU = TextureWrapMode::ClampToEdge;
		texBluePrint.Sampler.WrapV = TextureWrapMode::ClampToEdge;

		for (int idx = 0; idx < TextureAsset::TEXTURE_ASSET_MAX; ++idx) {
			Tex[idx] = Gfx::LoadResource(TextureLoader::Create(TextureSetup::FromFile(TexPaths[idx], texBluePrint)));
		}

		IO::Load(MapFilePath, [this](IO::LoadResult res) {
			const uint8_t* ptr = res.Data.Data();

			String str(ptr);
			MapFile mapFile;
			mapFile.Load(walls, sprites, str);

			MapFileLoaded = true;
		});
	}

	void Discard() {

	}

	bool DoneLoading() {
		if (Loaded && MapFileLoaded)
			return true;

		for (int idx = 0; idx < TextureAsset::TEXTURE_ASSET_MAX; ++idx) {
			const auto resState = Gfx::QueryResourceInfo(Tex[idx]).State;

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
