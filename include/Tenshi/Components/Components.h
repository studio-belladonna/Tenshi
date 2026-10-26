#pragma once

#include "Tenshi/Entity/Entity.h"
#include "Tenshi/Scene/Collider.h"

namespace Tenshi {

class Sprite;

struct Transform {
    double x = 0.0;
    double y = 0.0;
};

struct Velocity {
    double x = 0.0;
    double y = 0.0;
};

struct CharacterController {
    double moveAxis = 0.0;
    bool jumpPressed = false;

    double walkSpeed = 120.0;
    double jumpSpeed = 30.0;
    double gravity = 260.0;
    double terminalVelocity = 44.0;
};

struct CharacterCollider {
    Collider feet;
};

struct SpriteRenderer {
    Sprite* sprite = nullptr;
    bool visible = true;
    int facingDirection = 1;
};

struct PlayerControlled {};
struct CameraTarget {};

} // namespace Tenshi