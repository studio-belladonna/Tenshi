#pragma once

#include "Tenshi/Utility/Types.h"
#include "Tenshi/GUI/Element.h"

namespace Tenshi {
namespace GUI {

class Frame : public Element {
public:
    Frame(std::string id, Rect<float> bounds);
    
    void draw(Renderer& renderer) override;
};

} // namespace GUI
} // namespace Tenshi