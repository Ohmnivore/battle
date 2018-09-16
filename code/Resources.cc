#include "Resources.h"

#include "IO/IO.h"
#include "HttpFS/HTTPFileSystem.h"
#include "LocalFS/LocalFileSystem.h"

#include "MapFile.h"
#include "STBTextureLoader.h"

using namespace Oryol;


void Resources::Setup() {
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

    LoadLvl(MapFiles[CurMapIdx]);
}

void Resources::Discard() {

}


void Resources::SwitchLvl() {
    CurMapIdx++;
    if (CurMapIdx >= MAX_MAP_FILES)
        CurMapIdx = 0;

    LoadLvl(MapFiles[CurMapIdx]);
}


bool Resources::DoneLoading() {
    JustLoaded = false;

    if (!MapFileLoaded)
        return false;

    if (Loaded)
        return true;

    for (int idx = 0; idx < Tex.Size(); ++idx) {
        const auto resState = Gfx::QueryResourceInfo(Tex[idx]).State;
        if (resState != ResourceState::Valid)
            return false;
    }

    if (!Loaded)
        JustLoaded = true;

    Loaded = true;
    return true;
}

bool Resources::JustDoneLoading() {
    return JustLoaded;
}


void Resources::LoadLvl(const char* mapFile) {
    IO::Load(mapFile, [this](IO::LoadResult res) {
        Loaded = false;
        JustLoaded = false;

        const uint8_t* ptr = res.Data.Data();

        String str(ptr);
        MapFile mapFile;
        Lvl.Reset();
        mapFile.Load(Lvl, str);
        MapFileLoaded = true;

        TextureSetup texBluePrint;
        texBluePrint.Sampler.MinFilter = TextureFilterMode::Nearest;
        texBluePrint.Sampler.MagFilter = TextureFilterMode::Nearest;
        texBluePrint.Sampler.WrapU = TextureWrapMode::ClampToEdge;
        texBluePrint.Sampler.WrapV = TextureWrapMode::ClampToEdge;

        // Delete previous textures
        if (Tex.Size() > 0) {
            Gfx::DestroyResources(Label);
        }
        Tex.Clear();

        // Load new textures
        Gfx::PushResourceLabel();
        for (int idx = 0; idx < Lvl.texPaths.Size(); ++idx) {
            Tex.Add(
                Gfx::LoadResource(STBTextureLoader::Create(TextureSetup::FromFile(Lvl.texPaths[idx].AsCStr(), texBluePrint)))
            );
        }
        Label = Gfx::PopResourceLabel();
    });
}
