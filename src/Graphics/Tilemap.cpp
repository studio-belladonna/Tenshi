#include "Tenshi/Graphics/Tilemap.h"

#include "Tenshi/Scene/Camera.h"
#include "Tenshi/Graphics/Sprite.h"
#include "Tenshi/Graphics/SpriteBatch.h"
#include "Tenshi/Graphics/Tileset.h"
#include "Tenshi/Files/FileSystem.h"

#include "Graphics/TilemapFormat.h"

#include <algorithm>
#include <cstring>
#include <fstream>

namespace Tenshi {
namespace {

bool RangeValid(size_t fileSize, uint32_t offset, uint32_t size) {
    const size_t start = static_cast<size_t>(offset);
    const size_t length = static_cast<size_t>(size);

    if (start > fileSize)
        return false;

    if (length > fileSize - start)
        return false;

    return true;
}

const char* GetString(const std::vector<uint8_t>& data, uint32_t offset, uint32_t length) {
    if (!RangeValid(data.size(), offset, length)) {
        return nullptr;
    }

    return reinterpret_cast<const char*>(data.data() + offset);
}

SDL_FRect MakeRect(int x, int y, int w, int h) {
    SDL_FRect rect;
    rect.x = static_cast<float>(x);
    rect.y = static_cast<float>(y);
    rect.w = static_cast<float>(w);
    rect.h = static_cast<float>(h);

    return rect;
}

} // namespace

MapLayer::MapLayer(int width, int height)
    : width(width)
    , height(height)
{
    const size_t count = static_cast<size_t>(width) * height;

    tiles.resize(count, AIR_TILE);
    attributes.resize(count, 0);
}

TileId MapLayer::GetTile(int x, int y) const {
    if (x < 0 
    ||  y < 0 
    ||  x >= width 
    ||  y >= height
    ) {
        return AIR_TILE;
    }

    return tiles[
        static_cast<size_t>(y) * width + x
    ];
}

uint8_t MapLayer::GetAttributes(int x, int y) const {
    if (x < 0 
    ||  y < 0 
    ||  x >= width 
    ||  y >= height
    ) {
        return 0;
    }

    return attributes[
        static_cast<size_t>(y) * width + x
    ];
}

void MapLayer::SetTile(int x, int y, TileId tile) {
    if (x < 0 
    ||  y < 0 
    ||  x >= width 
    ||  y >= height
    ) {
        return;
    }

    tiles[static_cast<size_t>(y) * width + x] = tile;
}

void MapLayer::SetAttributes(int x, int y, uint8_t value) {
    if (x < 0 
    ||  y < 0 
    ||  x >= width 
    ||  y >= height
    ) {
        return;
    }

    attributes[static_cast<size_t>(y) * width + x] = value;
}

const std::vector<TileId>& MapLayer::GetTiles() const {
    return tiles;
}

const std::vector<uint8_t>& MapLayer::GetAttributes() const {
    return attributes;
}

std::vector<TileId>& MapLayer::GetTiles() {
    return tiles;
}

std::vector<uint8_t>& MapLayer::GetAttributes() {
    return attributes;
}

int MapLayer::GetWidth() const {
    return width;
}

int MapLayer::GetHeight() const {
    return height;
}

Tilemap::Tilemap(SDL_Renderer* renderer,int width, int height)
    : width(width)
    , height(height)
{
    batch = std::make_unique<SpriteBatch>(renderer);

    collision.resize(
        static_cast<size_t>(width) * height,
        static_cast<uint8_t>(TileColliderType::NONE)
    );
}

Tilemap::~Tilemap() = default;
bool Tilemap::Load(FileSystem& files, const std::string& path) {
    std::vector<uint8_t> data = files.readAll(path);

    if (data.empty())
        return false;

    if (data.size() < sizeof(TMapHeader))
        return false;

    const auto* header = reinterpret_cast<const TMapHeader*>(
        data.data()
    );

    if (std::memcmp(header->magic, "TMAP", 4) != 0) {
        return false;
    }

    if (header->version != TMAP_VERSION)
        return false;

    if (header->width == 0 
    ||  header->height == 0
    ) {
        return false;
    }

    if (header->layerCount == 0)
        return false;

    const size_t transitionEntrySize 
    =   static_cast<size_t>(header->entryCount) 
    *   sizeof(TMapTransition);

    if (!RangeValid(
            data.size(),
            header->entriesOffset,
            static_cast<uint32_t>(transitionEntrySize)
        )
    ) {
        return false;
    }

    const size_t transitionExitSize 
    =   static_cast<size_t>(header->exitCount) 
    *   sizeof(TMapTransition);

    if (!RangeValid(
            data.size(),
            header->exitsOffset,
            static_cast<uint32_t>(transitionExitSize)
        )
    ) {
        return false;
    }

    const size_t layerTableSize 
    =   static_cast<size_t>(header->layerCount) 
    *   sizeof(TMapLayer);

    if (!RangeValid(
            data.size(),
            header->layersOffset,
            static_cast<uint32_t>(layerTableSize)
        )
    ) {
        return false;
    }

    const char* mapNameData = GetString(
        data,
        header->mapNameOffset,
        header->mapNameLength
    );

    const char* tilesetNameData = GetString(
        data,
        header->tilesetNameOffset,
        header->tilesetNameLength
    );

    if (!mapNameData 
    ||  !tilesetNameData
    ) {
        return false;
    }

    width = header->width;
    height = header->height;

    name.assign(mapNameData, header->mapNameLength);
    tilesetName.assign(tilesetNameData, header->tilesetNameLength);

    entries.resize(header->entryCount);

    if (!entries.empty()) {
        std::memcpy(
            entries.data(), 
            data.data() + header->entriesOffset,
            entries.size() * sizeof(TMapTransition)
        );
    }

    exits.resize(header->exitCount);

    if (!exits.empty()) {
        std::memcpy(
            exits.data(),
            data.data() + header->exitsOffset,
            exits.size() * sizeof(TMapTransition)
        );
    }

    const size_t tileCount = static_cast<size_t>(width) * height;

    if (!RangeValid(
            data.size(),
            header->collisionOffset,
            static_cast<uint32_t>(tileCount)
        )
    ) {
        return false;
    }

    collision.resize(tileCount);

    std::memcpy(
        collision.data(), 
        data.data() + header->collisionOffset, 
        tileCount
    );

    const auto* layerHeaders 
    =   reinterpret_cast<const TMapLayer*>(
        data.data() + header->layersOffset
    );

    layers.clear();
    layers.reserve(header->layerCount);

    const size_t expectedTileBytes 
    =   tileCount 
    *   sizeof(TileId);

    const size_t expectedAttributeBytes 
    =   tileCount 
    *   sizeof(uint8_t);

    for (uint16_t i = 0; i < header->layerCount; ++i) {
        const TMapLayer& fileLayer = layerHeaders[i];

        if (fileLayer.tileDataSize 
        !=  expectedTileBytes
        ) {
            return false;
        }

        if (fileLayer.attributeDataSize 
        !=  expectedAttributeBytes
        ) {
            return false;
        }

        if (!RangeValid(
                data.size(),
                fileLayer.tileDataOffset,
                fileLayer.tileDataSize
            )
        ) {
            return false;
        }

        if (!RangeValid(
                data.size(),
                fileLayer.attributeDataOffset,
                fileLayer.attributeDataSize
            )
        ) {
            return false;
        }

        MapLayer layer(width, height);
        layer.depth = fileLayer.depth;

        std::memcpy(
            layer.GetTiles().data(),
            data.data() + fileLayer.tileDataOffset,
            fileLayer.tileDataSize
        );

        std::memcpy(
            layer.GetAttributes().data(),
            data.data() + fileLayer.attributeDataOffset,
            fileLayer.attributeDataSize
        );

        layers.emplace_back(std::move(layer));
    }

    ++collisionRevision;
    return true;
}

void Tilemap::Step(double deltaTime) {
    // step animation etc...
    (void)deltaTime;
}

void Tilemap::Draw(const Camera& camera) {
    if (!tileset) return;

    Sprite* sprite = tileset->GetSprite();
    if (!sprite) return;

    const int tileWidth = tileset->GetTileWidth();
    const int tileHeight = tileset->GetTileHeight();

    if (tileWidth <= 0 
    ||  tileHeight <= 0
    ) {
        return;
    }

    const int camX = camera.getViewX();
    const int camY = camera.getViewY();

    const int startX = std::max(
        0,
        camX / tileWidth
    );

    const int startY = std::max(
        0,
        camY / tileHeight
    );

    const int endX = std::min(
        width,
        (camX + camera.viewW) 
        /   tileWidth + 1
    );

    const int endY = std::min(
        height,
        (camY + camera.viewH) 
        /   tileHeight + 1
    );

    batch->begin();

    for (const MapLayer& layer : layers) {
        for (int y = startY; y < endY; ++y) {
            for (int x = startX; x < endX; ++x) {
                const TileId tileId = layer.GetTile(x, y);

                if (tileId == AIR_TILE) continue;

                const TilesetTile* tile = tileset->GetTile(tileId);
                if (!tile) continue;

                SDL_FRect source = MakeRect(
                    tile->x * tileWidth,
                    tile->y * tileHeight,
                    tileWidth,
                    tileHeight
                );

                SDL_FRect destination = MakeRect(
                    x * tileWidth - camX,
                    y * tileHeight - camY,
                    tileWidth,
                    tileHeight
                );

                batch->draw(
                    sprite->GetTexture(),
                    source,
                    destination,
                    0.0f,
                    { 0.0f, 0.0f },
                    SDL_FLIP_NONE
                );
            }
        }
    }

    batch->flush();
}

void Tilemap::SetTileset(Tileset* ts) {
    tileset = ts;
}

int Tilemap::GetWidth() const {
    return width;
}

int Tilemap::GetHeight() const {
    return height;
}

const std::string& Tilemap::GetName() const {
    return name;
}

const std::string& Tilemap::GetTilesetName() const {
    return tilesetName;
}

size_t Tilemap::GetLayerCount() const {
    return layers.size();
}

MapLayer* Tilemap::GetLayer(size_t index) {
    if (index >= layers.size())
        return nullptr;

    return &layers[index];
}

const MapLayer* Tilemap::GetLayer(size_t index) const {
    if (index >= layers.size())
        return nullptr;

    return &layers[index];
}

uint8_t Tilemap::GetCollision(int x, int y) const {
    if (x < 0 
    ||  y < 0 
    ||  x >= width 
    ||  y >= height
    ) {
        return static_cast<uint8_t>(TileColliderType::NONE);
    }

    return collision[static_cast<size_t>(y) * width + x];
}

void Tilemap::SetCollision(int x, int y, uint8_t type) {
    if (x < 0 
    ||  y < 0 
    ||  x >= width 
    ||  y >= height) {
        return;
    }

    uint8_t& current = collision[static_cast<size_t>(y) * width + x];
    if (current != type) {
        current = type;
        ++collisionRevision;
    }
}

uint64_t Tilemap::GetCollisionRevision() const {
    return collisionRevision;
}

const MapTransition* Tilemap::GetEntry(uint32_t id) const {
    for (const auto& entry : entries) {
        if (entry.id == id) return &entry;
    }

    return nullptr;
}

const MapTransition* Tilemap::GetExit(uint32_t id) const {
    for (const auto& exit : exits) {
        if (exit.id == id) return &exit;
    }

    return nullptr;
}

} // namespace Tenshi