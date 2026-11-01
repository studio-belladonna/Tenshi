#pragma once

#include "Tenshi/Graphics/Renderer.h"

#include <cstddef>
#include <cstdint>
#include <memory>

namespace Tenshi {

std::unique_ptr<GraphicsTexture> LoadTextureSTB(Renderer& renderer, const uint8_t* data, size_t size);

} // namespace Tenshi