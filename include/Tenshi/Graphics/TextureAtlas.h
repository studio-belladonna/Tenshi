#pragma once

#include "Tenshi/Graphics/Renderer.h"

#include <unordered_map>
#include <string>

namespace Tenshi {

struct AtlasRegion {
    GraphicsTexture* texture;

    Rect<float> region;

    int width;
    int height;
};

class TextureAtlas {
public:

    GraphicsTexture* texture;

    std::unordered_map<std::string, AtlasRegion> regions;

    AtlasRegion* get(const std::string& name) {
        return &regions[name];
    }
};

} // namespace Tenshi