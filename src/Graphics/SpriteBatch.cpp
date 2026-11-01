#include "Tenshi/Graphics/SpriteBatch.h"

#include <functional>

namespace Tenshi {

SpriteBatch::SpriteBatch(Renderer& renderer)
    : renderer(renderer)
{
}

void SpriteBatch::begin() {
    commands.clear();
}

void SpriteBatch::draw(
    GraphicsTexture* texture,
    const Rect<float>& src,
    const Rect<float>& dst,
    float rotation,
    Vec<float, 2> origin,
    TextureFlip flip
) {
    SpriteCommand cmd{
        texture,
        src,
        dst,
        rotation,
        origin,
        flip,
        0
    };

    commands.push_back(cmd);
}

void SpriteBatch::flush() {
    std::sort(commands.begin(), commands.end(),
        [](const SpriteCommand& a, const SpriteCommand& b) {
            if (a.layer != b.layer)
                return a.layer < b.layer;

            return std::less<GraphicsTexture*>{}(a.texture, b.texture);
        });

    for (const SpriteCommand& cmd : commands) {
        if (cmd.texture) {
            renderer.drawTexture(
                *cmd.texture,
                cmd.src,
                cmd.dst,
                cmd.rotation,
                cmd.origin,
                cmd.flip
            );
        }
    }
}

} // namespace Tenshi