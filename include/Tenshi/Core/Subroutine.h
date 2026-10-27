#pragma once

#include "Tenshi/Event/Event.h"

namespace Tenshi {
class Game;

class Subroutine {
public:
    virtual ~Subroutine() = default;

    virtual bool init() { return true; }
    virtual void event(const Event& event) {}
    virtual void update(double deltaTime) {}
    virtual void fixedUpdate(double dt) {}
    virtual void draw() {}

protected:
    Game* game = nullptr;

    explicit Subroutine(Game* parentGame) : game(parentGame)
    {
    }
};

} // namespace Tenshi