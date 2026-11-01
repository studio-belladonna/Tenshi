#pragma once

#include "Tenshi/Utility/Types.h"

#include <cstdint>
#include <memory>
#include <string>

namespace Tenshi {

struct RendererConfig {
    std::string title;
    int windowWidth = 0;
    int windowHeight = 0;
    int windowScale = 1;
    bool windowResizable = false;
};

enum class TextureFlip {
    None,
    Horizontal,
    Vertical
};

class GraphicsTexture {
public:
    virtual ~GraphicsTexture() = default;

    virtual int getWidth() const = 0;
    virtual int getHeight() const = 0;
};

class Renderer {
public:
    virtual ~Renderer() = default;

    virtual bool initialize(const RendererConfig& config) = 0;
    virtual void shutdown() = 0;

    virtual std::unique_ptr<GraphicsTexture> createTexture(
        const uint8_t* rgbaPixels,
        int width,
        int height
    ) = 0;

    virtual void setWindowSize(int width, int height) = 0;
    virtual void setLogicalSize(int width, int height) = 0;
    virtual void getOutputSize(int& width, int& height) const = 0;

    virtual void clear(rgba colour) = 0;
    virtual void present() = 0;
    virtual void fillRect(const Rect<float>& rect, rgba colour) = 0;
    virtual void drawTexture(
        const GraphicsTexture& texture,
        const Rect<float>& source,
        const Rect<float>& destination,
        float rotation,
        Vec<float, 2> origin,
        TextureFlip flip
    ) = 0;
    virtual void drawTextureTiled(
        const GraphicsTexture& texture,
        const Rect<float>& destination
    ) = 0;
};

std::unique_ptr<Renderer> CreateRenderer();

} // namespace Tenshi
