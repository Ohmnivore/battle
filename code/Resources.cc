#pragma once
#include "Core/Containers/Array.h"
#include "Gfx/Gfx.h"
#include "IO/IO.h"
#include "HttpFS/HTTPFileSystem.h"
#include "LocalFS/LocalFileSystem.h"

#include "MapFile.cc"
#include "Renderer.cc"
#include "STBTextureLoader.h"

using namespace Oryol;

class Resources {

public:

    static const int MAX_MAP_FILES = 3;
    const char* MapFiles[MAX_MAP_FILES] = {
        "assets:emerald_beach.map",
        "assets:holy_summit.map",
        "assets:battle_highway.map",
    };

    Oryol::Array<Oryol::Id> Tex;
    Renderer::LvlData Lvl;

    void Setup() {
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

    void SwitchLvl() {
        CurMapIdx++;
        if (CurMapIdx >= MAX_MAP_FILES)
            CurMapIdx = 0;

        LoadLvl(MapFiles[CurMapIdx]);
    }

    void Discard() {

    }

    bool DoneLoading() {
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

    bool JustDoneLoading() {
        return JustLoaded;
    }

private:

    bool Loaded = false;
    bool JustLoaded = false;
    bool MapFileLoaded = false;
    int CurMapIdx = 0;
    Oryol::ResourceLabel Label;

    void LoadLvl(const char* mapFile) {
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
};
