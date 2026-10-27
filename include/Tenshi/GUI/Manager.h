#pragma once

#include "Tenshi/Core/Subroutine.h"
#include "Tenshi/GUI/Element.h"
#include "Tenshi/Event/Event.h"

#include <SDL3/SDL.h>
#include <vector>
#include <memory>

namespace Tenshi {

class Game;

// acts as a root for all ui Elements
class UIManager : public Subroutine {
public:
    explicit UIManager(Game* game);
    ~UIManager();

    void event(const Event& event);
    void update(double deltaTime);
    void fixedUpdate(double deltaTime);
    void draw();

    void pushChild(std::unique_ptr<GUI::Element> child);
private:
    std::vector<std::unique_ptr<GUI::Element>> children;
};


} // namespace Tenshi