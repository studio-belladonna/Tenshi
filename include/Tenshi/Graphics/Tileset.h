#pragma once

#include <cstdint>
#include <vector>

#include "Tenshi/Graphics/Sprite.h"
#include "Tenshi/Graphics/TileTypes.h"

namespace Tenshi {

class Tileset {
public:
    Tileset();
    ~Tileset();

    bool SetSprite(Sprite* spr);

    // load tileset data from a .ena file.
    bool LoadFromData(const char* dataPath);

    uint8_t GetTileWidth() const;
    uint8_t GetTileHeight() const;

    Sprite* GetSprite();

    // returns nullptr if tileid is invalid.
    const TilesetTile* GetTile(TileId tileid) const;

    size_t GetTileCount() const;

private:
    std::vector<TilesetTile> tiles;

    Sprite* sprite = nullptr;

    uint8_t tileWidth = 16;
    uint8_t tileHeight = 16;

    bool animated = false;
};

} // namespace Tenshi
