#pragma once

#include "Tenshi/Graphics/Renderer.h"

#include <vector>
#include <algorithm>

namespace Tenshi {

struct SpriteCommand {
    GraphicsTexture* texture;

    Rect<float> src;
    Rect<float> dst;

    float rotation;

    Vec<float, 2> origin;

    TextureFlip flip;

    int layer;
};

struct BatchGroup {
    GraphicsTexture* texture;

    std::vector<SpriteCommand> sprites;
};

class SpriteBatch {
public:

    explicit SpriteBatch(Renderer& renderer);

    void begin();
    void draw(
        GraphicsTexture* texture,
        const Rect<float>& src,
        const Rect<float>& dst,
        float rotation = 0.0f,
        Vec<float, 2> origin = { 0, 0 },
        TextureFlip flip = TextureFlip::None
    );

    void flush();

private:
    Renderer& renderer;

    std::vector<SpriteCommand> commands;
};

} // namespace Tenshi