#include "Tenshi/GUI/Frame.h"

namespace Tenshi {
namespace GUI {

Frame::Frame(std::string id, Rect<float> bounds)
:   Element(std::move(id), bounds)
{
}

void Frame::draw(Renderer& renderer) {
    if (!active) return;

    renderer.fillRect(bounds, { 0, 0, 0, 255 });

    Element::draw(renderer);
}

} // namespace GUI
} // namespace Tenshi