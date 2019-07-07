#include "Renderer.h"

#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtx/matrix_transform_2d.hpp"

#include <algorithm>


// Constants
const float Renderer::SCREEN_WIDTH = 240.0f;
const float Renderer::SCREEN_HEIGHT = 160.0f;
const float Renderer::MAP_AND_WALL_SCALE = 2.0f;


// Renderable
Renderer::Renderable::Renderable()
{
}

Renderer::Renderable::Renderable(const Wall& wall, const glm::vec3& viewSpacePos, const glm::mat3& transform) :
    texIdx(wall.texIdx),
    pos(wall.pos),
    viewSpacePos(viewSpacePos),
    transform(transform)
{
}

Renderer::Renderable::Renderable(const Sprite& sprite, const glm::vec3& viewSpacePos, const glm::mat3& transform) :
    texIdx(sprite.texIdx),
    pos(sprite.pos),
    viewSpacePos(viewSpacePos),
    transform(transform)
{
}

Renderer::Renderable::Renderable(const DropShadow& dropShadow, const glm::mat3& transform, int texIdx) :
    texIdx(texIdx),
    transform(transform)
{
}


// LvlData
void Renderer::LvlData::Reset() {
    texPaths.Clear();
    texSizes.Clear();

    for (int dir = 0; dir < MAX_SIDE_WALL_DIRECTIONS; ++dir) {
        walls.walls[dir].Clear();
    }
    sprites.Clear();
    dropShadows.Clear();
    boxColliders.Clear();
}


// Renderer
void Renderer::Setup(LvlData& lvl) {
    SortedDropShadows.SetFixedCapacity(lvl.dropShadows.Size());

    int numRenderables = lvl.sprites.Size();

    for (int dir = 0; dir < MAX_SIDE_WALL_DIRECTIONS; ++dir) {
        numRenderables += lvl.walls.walls[dir].Size();
    }

    Sorted.SetFixedCapacity(numRenderables);
}


void Renderer::Update(Camera& cam, LvlData& lvl) {
    glm::mat3 scale = glm::scale(glm::mat3(), glm::vec2(1.0, glm::cos(cam.Pitch)) * MAP_AND_WALL_SCALE);
    glm::mat3 rotate = glm::rotate(scale, -cam.Heading);
    TileMapAffine = rotate;

    UpdateWalls(
        cam.GetDirXY(),
        cam.GetTransformInverse(),
        lvl,
        WallDirection::Y_PLUS
    );

    UpdateWalls(
        cam.GetDirXYTwisted(),
        cam.GetTransformInverseTwisted(),
        lvl,
        WallDirection::MAX_WALL_DIRECTIONS
    );
}


void Renderer::UpdateWalls(
    const glm::vec2& camDirXY,
    const glm::mat4& camTransformInverse,
    const LvlData& lvl,
    const WallDirection offset
)
{
    {
        WallVisible[WallDirection::Y_PLUS + offset] = camDirXY.x < 0.0f;
        WallVisible[WallDirection::Y_MINUS + offset] = camDirXY.x > 0.0f;
        WallVisible[WallDirection::X_PLUS + offset] = camDirXY.y > 0.0f;
        WallVisible[WallDirection::X_MINUS + offset] = camDirXY.y < 0.0f;
    }

    // Wall tangents for every WallDirection (in world space)
    // Skipped for twisted walls - it's the camera that twists 45 degrees instead
    glm::vec3 wallTangents[] = {
        glm::vec3(-1.0f,  0.0f, 0.0f), // Y_PLUS
        glm::vec3( 0.0f, -1.0f, 0.0f), // X_MINUS
        glm::vec3( 1.0f,  0.0f, 0.0f), // Y_MINUS
        glm::vec3( 0.0f,  1.0f, 0.0f), // X_PLUS
    };
    glm::vec3 wallBiTangent = glm::vec3( 0.0f, 0.0f, 1.0f ); // Same for all directions, *-1 to account for flipped vertical in texture space

    for (int dir = offset; dir < WallDirection::MAX_WALL_DIRECTIONS + offset; ++dir) {
        if (WallVisible[dir]) {
            // Transform the wall local basis from world to view space
            glm::mat2x3 basis(wallTangents[dir - offset], wallBiTangent);
            basis = glm::mat3(camTransformInverse) * basis;

            // Project to 2D - now we know how to transform textures in local wall space
            glm::mat3 affine(1.0f);
            affine[0][0] = basis[0][0];
            affine[0][1] = basis[0][1];
            affine[1][0] = basis[1][0];
            affine[1][1] = basis[1][1];

            // Apply wall height scale and base scale
            affine = glm::scale(affine, glm::vec2(1.0f, lvl.wallHeightMultiplier) * MAP_AND_WALL_SCALE);

            // Save affine matrix
            WallAffine[dir] = affine;
        }
    }
}


Renderer::TilemapList& Renderer::UpdateTilemaps(Camera& cam, LvlData& lvl) {
    for (int layer = TILEMAP_BOTTOM; layer < MAX_TILEMAP_LAYERS; ++layer) {
        Tilemap& map = lvl.tilemaps[layer];

        glm::vec4 modelPos(map.pos.x, map.pos.y, map.pos.z, 1.0f);
        glm::vec4 modelPosInViewSpace = cam.GetTransformInverse() * modelPos;

        glm::mat3 modelTranslate = glm::translate(glm::mat3(), glm::vec2(modelPosInViewSpace.x, modelPosInViewSpace.y));

        Renderable& dst = Tilemaps[layer];
        dst.transform = modelTranslate * TileMapAffine;
        dst.texIdx = map.texIdx;
    }

    return Tilemaps;
}


Renderer::SortedRenderList& Renderer::UpdateWallsAndSprites(Camera& cam, LvlData& lvl, int& numTopSprites) {
    int numSorted = 0;
    numTopSprites = 0;
    Sorted.Clear();

    for (int dir = 0; dir < WallDirection::MAX_SIDE_WALL_DIRECTIONS; ++dir) {
        if (WallVisible[dir]) {
            for (int wallIdx = 0; wallIdx < lvl.walls.walls[dir].Size(); ++wallIdx) {
                Wall& wall = lvl.walls.walls[dir][wallIdx];

                // Half-height wall offset
                float wallZOffset = lvl.texSizes[wall.texIdx].y / 2.0f * lvl.wallHeightMultiplier * MAP_AND_WALL_SCALE;

                // Compute view-space position
                glm::vec4 modelPos(wall.pos.x, wall.pos.y, wall.pos.z + wallZOffset, 1.0f);
                glm::vec4 modelPosInViewSpace = cam.GetTransformInverse() * modelPos;

                // Cull
                float xScale = WallAffine[wall.dir][0].x;
                float yScale = WallAffine[wall.dir][1].y;
                float slantFactor = glm::abs(WallAffine[wall.dir][0].y);
                float cullWidth = lvl.texSizes[wall.texIdx].x * xScale;
                float cullHeight = lvl.texSizes[wall.texIdx].y * yScale;
                cullHeight += cullWidth * slantFactor;
                if (!RectangleIsInViewFrustum(cullWidth, cullHeight, modelPosInViewSpace))
                {
                    continue;
                }

                // Compute transform matrix
                glm::mat3 modelTranslate = glm::translate(glm::mat3(), glm::vec2(modelPosInViewSpace.x, modelPosInViewSpace.y));
                glm::mat3 transform = modelTranslate * WallAffine[wall.dir];

                // Add & in-place sort based on view space Z position
                Renderable rend(wall, glm::vec3(modelPosInViewSpace), transform);
                auto insertPoint = std::upper_bound(Sorted.begin(), Sorted.begin() + numSorted, rend, &Renderer::RenderableDepthCompare);
                int idx = std::distance(Sorted.begin(), insertPoint);
                Sorted.Insert(idx, rend);

                numSorted++;
            }
        }
    }

    // Flip sprites horizontally if looking towards -Y axis
    glm::mat3 spriteFlip;
    if (cam.GetDir().y < 0.0f)
        spriteFlip = glm::scale(glm::mat3(), glm::vec2(-1.0f, 1.0f));

    for (int spriteIdx = 0; spriteIdx < lvl.sprites.Size(); ++spriteIdx) {
        Sprite& sprite = lvl.sprites[spriteIdx];

        // Compute view-space position
        glm::vec4 modelPos(sprite.pos.x, sprite.pos.y, sprite.pos.z, 1.0f);
        glm::vec4 modelPosInViewSpace = cam.GetTransformInverse() * modelPos;
        modelPosInViewSpace.y += lvl.texSizes[sprite.texIdx].y / 2.0f; // Offset by half-height

        // Cull
        float cullWidth = lvl.texSizes[sprite.texIdx].x;
        float cullHeight = lvl.texSizes[sprite.texIdx].y;
        if (!RectangleIsInViewFrustum(cullWidth, cullHeight, modelPosInViewSpace))
        {
            continue;
        }

        // Compute transform matrix
        glm::mat3 transform = glm::translate(glm::mat3(), glm::vec2(modelPosInViewSpace.x, modelPosInViewSpace.y)) * spriteFlip;

        bool top = sprite.pos.z >= lvl.tilemaps[TILEMAP_TOP].pos.z - lvl.floorSortOffset;

        if (top) {
            // Add & in-place sort based on view space Z position
            Renderable rend(sprite, glm::vec3(modelPosInViewSpace), transform);
            auto insertPoint = std::upper_bound(Sorted.end() - numTopSprites, Sorted.end(), rend, &Renderer::RenderableDepthCompare);
            int idx = std::distance(Sorted.begin(), insertPoint);
            Sorted.Insert(idx, rend);

            numTopSprites++;
        }
        else {
            // Add & in-place sort based on view space Z position
            Renderable rend(sprite, glm::vec3(modelPosInViewSpace), transform);
            auto insertPoint = std::upper_bound(Sorted.begin(), Sorted.begin() + numSorted, rend, &Renderer::RenderableDepthCompare);
            int idx = std::distance(Sorted.begin(), insertPoint);
            Sorted.Insert(idx, rend);

            numSorted++;
        }
    }

    return Sorted;
}


// Drop shadow constants
const float DROP_SHADOW_COLLISION_RADIUS = 8.0f;
const float DROP_SHADOW_MIN_SCALE = 0.5f;
const float DROP_SHADOW_SCALE_RANGE = 200.0f;
const float DROP_SHADOW_Y_OFFSET = 1.0f;

Renderer::SortedRenderList& Renderer::UpdateDropShadows(Camera& cam, LvlData& lvl, int& numFloorHeightShadows) {
    int numSorted = 0;
    numFloorHeightShadows = 0;
    SortedDropShadows.Clear();

    for (int shadowIdx = 0; shadowIdx < lvl.dropShadows.Size(); ++shadowIdx) {
        DropShadow& shadow = lvl.dropShadows[shadowIdx];

        shadow.pos.x = shadow.sprite->pos.x;
        shadow.pos.y = shadow.sprite->pos.y;

        bool secondFloor = false;

        for (int boxIdx = 0; boxIdx < lvl.boxColliders.Size(); ++boxIdx) {
            const BoxCollider& box = lvl.boxColliders[boxIdx];

            if (CollideCircleBox2D(glm::vec2(shadow.pos.x, shadow.pos.y), DROP_SHADOW_COLLISION_RADIUS, box)) {
                secondFloor = true;
                break;
            }
        }
            
        if (secondFloor) {
            shadow.pos.z = lvl.tilemaps[TILEMAP_TOP].pos.z;

            // Bump sprite z coord to second floor
            // (just for show)
            if (shadow.sprite->pos.z < shadow.pos.z) {
                shadow.sprite->pos.z = shadow.pos.z;
            }
        }
        else {
            shadow.pos.z = lvl.tilemaps[TILEMAP_BOTTOM].pos.z;
        }

        // Compute scale
        float shadowScale = glm::max(DROP_SHADOW_MIN_SCALE, 1.0f - (shadow.sprite->pos.z - shadow.pos.z) / DROP_SHADOW_SCALE_RANGE);
        glm::mat3 scale = glm::scale(glm::mat3(), glm::vec2(shadowScale, shadowScale));

        // Compute view-space position
        glm::vec4 modelPos(shadow.pos.x, shadow.pos.y, shadow.pos.z, 1.0f);
        glm::vec4 modelPosInViewSpace = cam.GetTransformInverse() * modelPos;
        modelPosInViewSpace.y += DROP_SHADOW_Y_OFFSET;

        // Compute transform matrix
        glm::mat3 transform = glm::translate(glm::mat3(), glm::vec2(modelPosInViewSpace.x, modelPosInViewSpace.y)) * scale;

        Renderable rend(shadow, transform, lvl.dropShadowTexIdx);
        if (secondFloor) {
            SortedDropShadows.Insert(SortedDropShadows.Size() - numFloorHeightShadows, rend);
            numFloorHeightShadows++;
        }
        else {
            SortedDropShadows.Insert(numSorted, rend);
            numSorted++;
        }
    }

    return SortedDropShadows;
}


// Utils
bool Renderer::RenderableDepthCompare(const Renderable& left, const Renderable& right) {
    return left.viewSpacePos.z < right.viewSpacePos.z;
}


bool Renderer::RectangleIsInViewFrustum(float width, float height, glm::vec4 position) {
    bool leftSideOverRightBound = (position.x - width / 2.0f) > SCREEN_WIDTH / 2.0f;
    bool rightSideOverLeftBound = (position.x + width / 2.0f) < -SCREEN_WIDTH / 2.0f;
    bool downSideOverUpBound = (position.y - height / 2.0f) > SCREEN_HEIGHT / 2.0f;
    bool upSideOverDownBound = (position.y + height / 2.0f) < -SCREEN_HEIGHT / 2.0f;
    
    return !leftSideOverRightBound && !rightSideOverLeftBound && !downSideOverUpBound && !upSideOverDownBound;
}


// Implements https://gamedev.stackexchange.com/a/120897
bool Renderer::CollideCircleBox2D(glm::vec2 circlePos, float circleRadius, BoxCollider box) {
    float r1x = box.pos.x - circleRadius;
    float r1y = box.pos.y;
    float r1w = box.size.x + circleRadius * 2.0f;
    float r1h = box.size.y;

    if (circlePos.x >= r1x && circlePos.x <= r1x + r1w &&
        circlePos.y >= r1y && circlePos.y <= r1y + r1h) {
        return true;
    }

    float r2x = box.pos.x;
    float r2y = box.pos.y - circleRadius;
    float r2w = box.size.x;
    float r2h = box.size.y + circleRadius * 2.0f;

    if (circlePos.x >= r2x && circlePos.x <= r2x + r2w &&
        circlePos.y >= r2y && circlePos.y <= r2y + r2h) {
        return true;
    }
        
    const glm::vec2 circleCenters[] = {
        glm::vec2(box.pos.x, box.pos.y),
        glm::vec2(box.pos.x, box.pos.y + box.size.y),
        glm::vec2(box.pos.x + box.size.x, box.pos.y + box.size.y),
        glm::vec2(box.pos.x + box.size.x, box.pos.y)
    };

    for (int circleIdx = 0; circleIdx < 4; ++circleIdx) {
        const glm::vec2& circleCenter = circleCenters[circleIdx];

        float dx = circlePos.x - circleCenter.x;
        float dy = circlePos.y - circleCenter.y;
        float dist = glm::sqrt(dx * dx + dy * dy);

        if (dist <= circleRadius)
            return true;
    }

    return false;
}
