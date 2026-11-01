#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "Tenshi/Graphics/TileTypes.h"

namespace Tenshi {

class Camera;
class FileSystem;
class Renderer;
class SpriteBatch;
class Tileset;
using TileFlags = uint8_t;

struct MapTransition {
    uint32_t id = 0;
    uint32_t tileIndex = 0;
};

class MapLayer {
public:
    MapLayer() = default;

    MapLayer(int width, int height);

    TileId GetTile(int x, int y) const;
    uint8_t GetAttributes(int x, int y) const;

    void SetTile(int x, int y, TileId tile);
    void SetAttributes(int x, int y, uint8_t attributes);

    const std::vector<TileId>& GetTiles() const;
    const std::vector<uint8_t>& GetAttributes() const;

    std::vector<TileId>& GetTiles();
    std::vector<uint8_t>& GetAttributes();

    int GetWidth() const;
    int GetHeight() const;

    int16_t depth = 0;
private:
    int width = 0;
    int height = 0;

    std::vector<TileId> tiles;
    std::vector<uint8_t> attributes;
};

class Tilemap {
public:
    Tilemap(Renderer& renderer, int width, int height);
    ~Tilemap();

    bool Load(FileSystem& files, const std::string& path);

    void Step(double deltaTime);
    void Draw(const Camera& camera);

    void SetTileset(Tileset* tileset);

    int GetWidth() const;
    int GetHeight() const;

    const std::string& GetName() const;
    const std::string& GetTilesetName() const;

    size_t GetLayerCount() const;

    MapLayer* GetLayer(size_t index);
    const MapLayer* GetLayer(size_t index) const;

    uint8_t GetCollision(int x, int y) const;
    void SetCollision(int x, int y, uint8_t type);
    uint64_t GetCollisionRevision() const;

    const MapTransition* GetEntry(uint32_t id) const;
    const MapTransition* GetExit(uint32_t id) const;

private:
    std::unique_ptr<SpriteBatch> batch;

    Tileset* tileset = nullptr;

    int width = 0;
    int height = 0;

    std::string name;
    std::string tilesetName;

    std::vector<MapLayer> layers;

    std::vector<uint8_t> collision;
    uint64_t collisionRevision = 0;

    std::vector<MapTransition> entries;
    std::vector<MapTransition> exits;
};

} // namespace Tenshi
