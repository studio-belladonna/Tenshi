#include "Image.h"

#include "Tenshi/Core/Log.h"

#include <vector>
#include <memory>
#include <limits>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

namespace Tenshi {

struct StbiDeleter {
    void operator()(unsigned char* pixels) const {
        stbi_image_free(pixels);
    }
};

std::unique_ptr<GraphicsTexture> LoadTextureSTB(Renderer& renderer, const uint8_t* data, size_t size) {
    if (!data || size == 0) {
        LogWarning(LogCategory::Resource, "Attempt to load null image data");
        return nullptr;
    }

    if (size > static_cast<size_t>(std::numeric_limits<int>::max())) {
        LogWarning(LogCategory::Resource, "Image data is too large to decode");
        return nullptr;
    }

    int width = 0;
    int height = 0;
    int channels = 0;
    std::unique_ptr<unsigned char, StbiDeleter> pixels(
        stbi_load_from_memory(
            data,
            static_cast<int>(size),
            &width, 
            &height, 
            &channels, 
            STBI_rgb_alpha
        )
    );

    if (!pixels) {
        LogWarning(LogCategory::Resource, "Could not read image data");
        return nullptr;
    }

    auto texture = renderer.createTexture(pixels.get(), width, height);
    if (!texture) {
        LogWarning(LogCategory::Resource, "Could not create texture for decoded image");
    }

    return texture;
}

} // namespace Tenshi