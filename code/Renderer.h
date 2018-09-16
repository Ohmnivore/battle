#pragma once
#include "Core/Containers/Array.h"
#include "Core/String/String.h"

#include "glm/mat4x4.hpp"

#include "Camera.h"


const float SCREEN_WIDTH = 240.0f;
const float SCREEN_HEIGHT = 160.0f;
const float MAP_AND_WALL_SCALE = 2.0f;

class Renderer {

public:

    enum WallDirection {
        Y_PLUS = 0,
        X_MINUS,
        Y_MINUS,
        X_PLUS,
        MAX_WALL_DIRECTIONS
    };

    struct Wall {
        glm::vec3 pos;
        int dir;
        int texIdx;
    };

    typedef Oryol::Array<Wall> WallsOfDir;
    struct AllWalls {
        WallsOfDir walls[WallDirection::MAX_WALL_DIRECTIONS];
    };

    struct Sprite {
        glm::vec3 pos;
        int texIdx;
        bool useDropShadow;
    };

    typedef Oryol::Array<Sprite> Sprites;

    struct DropShadow {
        Sprite* sprite;
        glm::vec3 pos;
    };

    typedef Oryol::Array<DropShadow> DropShadows;

    struct BoxCollider {
        glm::vec2 pos;
        glm::vec2 size;
    };

    typedef Oryol::Array<BoxCollider> BoxColliders;

    struct Renderable {

        Renderable(const Wall& wall, const glm::vec3& viewSpacePos, const glm::mat3& transform);

        Renderable(const Sprite& sprite, const glm::vec3& viewSpacePos, const glm::mat3& transform);

        Renderable(const DropShadow& dropShadow, const glm::mat3& transform, int texIdx);

        int texIdx;
        glm::vec3 pos;
        glm::vec3 viewSpacePos;
        glm::mat3 transform;
    };

    typedef Oryol::Array<Renderable> SortedRenderList;

    enum TilemapOrder {
        TILEMAP_BOTTOM,
        TILEMAP_TOP,
        MAX_TILEMAP_ORDERS
    };

    struct Tilemap {
        glm::vec3 pos;
        int texIdx;
    };

    typedef Oryol::Array<Oryol::String> TexPaths;
    typedef Oryol::Array<glm::vec2> TexSizes;

    struct LvlData {

        void Reset();

        TexPaths texPaths;
        TexSizes texSizes;

        Tilemap tilemaps[MAX_TILEMAP_ORDERS];
        AllWalls walls;
        Sprites sprites;
        DropShadows dropShadows;
        BoxColliders boxColliders;

        float floorSortOffset;
        float floorHeight;
        float wallGfxHeight;
        float wallHeightMultiplier;

        int dropShadowTexIdx;
        glm::vec2 camOffset;
        int bgColor[3];
    };

    void Setup(LvlData& lvl);

    void Update(Camera& cam, LvlData& lvl);

    void RenderTilemap(Camera& cam, const glm::vec3& pos, glm::mat3& dst);

    SortedRenderList& Sort(Camera& cam, LvlData& lvl, int& numTopSprites);

    SortedRenderList& UpdateDropShadows(Camera& cam, LvlData& lvl, int& numFloorHeightShadows);

private:

    static bool RenderableDepthCompare(const Renderable& left, const Renderable& right);

    static bool CollideCircleBox2D(glm::vec2 circlePos, float circleRadius, BoxCollider box);

    glm::mat3 TileMapAffine;
    glm::mat3 WallAffine[WallDirection::MAX_WALL_DIRECTIONS];
    glm::vec2 WallScale[WallDirection::MAX_WALL_DIRECTIONS];

    float WallDot[WallDirection::MAX_WALL_DIRECTIONS];
    float WallShear[WallDirection::MAX_WALL_DIRECTIONS];
    bool WallVisible[WallDirection::MAX_WALL_DIRECTIONS];

    SortedRenderList Sorted;
    SortedRenderList SortedDropShadows;
};
