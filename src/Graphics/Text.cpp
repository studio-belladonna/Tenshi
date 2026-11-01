#include "Tenshi/Graphics/Text.h"

#include "Tenshi/Assets/Font.h"
#include "Tenshi/Core/Log.h"

#include <cmath>
#include <cstdint>
#include <vector>
#include <algorithm>

namespace Tenshi {
namespace {

std::vector<uint32_t> DecodeUtf8(const std::string& utf8) {
    std::vector<uint32_t> result;
    for (size_t i = 0; i < utf8.size();) {
        const auto first = static_cast<uint8_t>(utf8[i]);
        if (first < 0x80) {
            result.push_back(first); ++i; continue;
        }

        int length = 0;
        uint32_t codepoint = 0;
        uint32_t minimum = 0;

        if ((first & 0xE0) == 0xC0) { 
            length = 2;
            codepoint = first & 0x1F;
            minimum = 0x80; 
        } else if ((first & 0xF0) == 0xE0) {
            length = 3;
            codepoint = first & 0x0F;
            minimum = 0x800; 
        } else if ((first & 0xF8) == 0xF0) {
            length = 4;
            codepoint = first & 0x07;
            minimum = 0x10000; 
        }

        bool valid = length != 0 && i + static_cast<size_t>(length) <= utf8.size();
        for (int j = 1; valid && j < length; ++j) {
            const auto next = static_cast<uint8_t>(utf8[i + j]);
            if ((next & 0xC0) != 0x80) { 
                valid = false;
            } else {
                codepoint = (codepoint << 6) | (next & 0x3F);
            }
        }

        valid = valid
        &&  codepoint >= minimum
        &&  codepoint <= 0x10FFFF
        &&  !(codepoint >= 0xD800 && codepoint <= 0xDFFF);

        result.push_back(valid ? codepoint : 0xFFFD);

        i += valid ? static_cast<size_t>(length) : 1;
    }
    return result;
}

struct PositionedGlyph {
    Font::GlyphBitmap bitmap;

    int x = 0;
    int baseline = 0;
};

} // namespace

TextRenderer::TextRenderer() {
    rect = { 0.0f, 0.0f, 0.0f, 0.0f };
    colour = { 255, 255, 255, 255 };
}

TextRenderer::~TextRenderer() = default;

void TextRenderer::Draw() {
    if (!renderer)
        return;

    if (dirty)
        RebuildTexture();

    if (texture) {
        const Rect<float> source{
            0.0f,
            0.0f,
            static_cast<float>(texture->getWidth()),
            static_cast<float>(texture->getHeight())
        };
        renderer->drawTexture(
            *texture,
            source,
            rect,
            0.0f,
            { 0.0f, 0.0f },
            TextureFlip::None
        );
    }
}

void TextRenderer::RebuildTexture() {
    texture.reset();

    if (text.empty()) {
        rect.w = 0.0f;
        rect.h = 0.0f;
        textureOffsetX = 0.0f;

        dirty = false;
        return;
    }

    if (!renderer) {
        LogError(LogCategory::Renderer, "TextRenderer has no Renderer");
        dirty = false;
        return;
    }

    if (!font || !font->IsLoaded()) {
        LogError(LogCategory::Renderer, "TextRenderer has no loaded font");
        dirty = false;
        return;
    }

    const auto codepoints = DecodeUtf8(text);
    const int lineSkip = font->GetLineSkip();
    const int ascent = font->GetAscent();

    std::vector<PositionedGlyph> glyphs;
    float penX = 0.0f;
    int baseline = ascent;
    float maxAdvance = 0.0f;
    int maxBottom = lineSkip;
    uint32_t previous = 0;
    bool hasPrevious = false;

    for (uint32_t codepoint : codepoints) {
        if (codepoint == '\r') continue;

        if (codepoint == '\n') {
            maxAdvance = std::max(maxAdvance, penX);
            penX = 0.0f;
            baseline += lineSkip;
            maxBottom += lineSkip;
            hasPrevious = false;

            continue;
        }

        if (codepoint == '\t') {
            codepoint = ' ';
            penX += font->GetAdvance(' ') * 3.0f;
        }

        if (hasPrevious) penX += font->GetKerningAdvance(previous, codepoint);

        Font::GlyphBitmap bitmap;
        font->RasterizeGlyph(codepoint, bitmap);
        glyphs.push_back({
            std::move(bitmap), static_cast<int>(std::floor(penX)), baseline
        });

        penX += font->GetAdvance(codepoint);
        maxAdvance = std::max(maxAdvance, penX);
        previous = codepoint;

        hasPrevious = true;
    }

    int minX = 0;
    int maxX = static_cast<int>(std::ceil(maxAdvance));
    for (const auto& glyph : glyphs) {
        minX = std::min(
            minX,
            glyph.x + glyph.bitmap.xOffset
        );

        maxX = std::max(
            maxX,
            glyph.x + glyph.bitmap.xOffset + glyph.bitmap.width
        );

        maxBottom = std::max(
            maxBottom,
            glyph.baseline + glyph.bitmap.yOffset + glyph.bitmap.height
        );
    }

    const int width = std::max(1, maxX - minX);
    const int height = std::max(1, maxBottom);

    std::vector<uint8_t> pixels(
        static_cast<size_t>(width) * static_cast<size_t>(height) * 4
    );

    for (const auto& glyph : glyphs) {
        for (int gy = 0; gy < glyph.bitmap.height; ++gy) {
            const int surfaceY = glyph.baseline + glyph.bitmap.yOffset + gy;
            if (surfaceY < 0 || surfaceY >= height) continue;
            
            for (int gx = 0; gx < glyph.bitmap.width; ++gx) {
                const int surfaceX = glyph.x + glyph.bitmap.xOffset + gx - minX;
                if (surfaceX < 0 || surfaceX >= width) continue;

                const uint8_t coverage = glyph.bitmap.pixels[gy * glyph.bitmap.width + gx];
                if (!coverage) continue;
                
                const uint8_t glyphAlpha = antialiased
                    ? coverage
                    : (coverage >= 128 ? 255 : 0);
                
                const uint8_t alpha = static_cast<uint8_t>((glyphAlpha * colour.a + 127) / 255);

                const size_t offset
                =   (static_cast<size_t>(surfaceY) * width + surfaceX) * 4;
                pixels[offset] = static_cast<uint8_t>(colour.r);
                pixels[offset + 1] = static_cast<uint8_t>(colour.g);
                pixels[offset + 2] = static_cast<uint8_t>(colour.b);
                pixels[offset + 3] = alpha;
            }
        }
    }

    texture = renderer->createTexture(pixels.data(), width, height);
    if (!texture) {
        LogError(LogCategory::Renderer, "Could not create text texture");
        return;
    }

    textureOffsetX = static_cast<float>(minX);
    rect.x = std::trunc(x) + textureOffsetX;
    rect.y = std::trunc(y);
    rect.w = static_cast<float>(width);
    rect.h = static_cast<float>(height);

    dirty = false;
}

void TextRenderer::SubmitText(const std::string& t) {
    if (text != t) {
        text = t;
        dirty = true;
    }
}

void TextRenderer::SetPosition(float newX, float newY) {
    x = newX;
    y = newY;

    rect.x = x + textureOffsetX;
    rect.y = y;
}

void TextRenderer::SetColour(rgba col) {
    colour = col;
    dirty = true;
}

void TextRenderer::SetAntialiasing(bool enabled) {
    if (antialiased != enabled) {
        antialiased = enabled;
        dirty = true;
    }
}

void TextRenderer::SetFont(Font* f) {
    if (font != f) {
        font = f;
        dirty = true;
    }
}

void TextRenderer::SetRenderer(Renderer& newRenderer) {
    if (renderer == &newRenderer)
        return;

    renderer = &newRenderer;
    dirty = true;
    texture.reset();
}

void TextRenderer::ClearText() {
    text.clear();

    texture.reset();

    rect.w = 0.0f;
    rect.h = 0.0f;
    textureOffsetX = 0.0f;

    dirty = false;
}

std::string TextRenderer::GetText() {
    return text;
}

rgba TextRenderer::GetColour() {
    return colour;
}

std::unique_ptr<TextRenderer> MakeTextRenderer(Renderer& renderer, Font* font) {
    auto new_TextRenderer = std::make_unique<TextRenderer>();

    new_TextRenderer->SetRenderer(renderer);
    new_TextRenderer->SetFont(font);

    return new_TextRenderer;
}

} // namespace Tenshi