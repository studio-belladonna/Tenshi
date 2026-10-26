#include "Tenshi/Character/Character.h"

#include <algorithm>
#include <stdexcept>

namespace Tenshi {

Character::Character(World& owner, Sprite* sprite, double x, double y)
	: world(&owner)
{
	if (!sprite)
		throw std::invalid_argument("A character requires a sprite");

	entity = world->create();

	try {
		world->emplace<Transform>(entity, x, y);
		world->emplace<Velocity>(entity);
		world->emplace<CharacterController>(entity);

		CharacterCollider collider;
		collider.feet.w = 6;
		collider.feet.h = 8;
		collider.feet.offsetX = (sprite->getWidth() / 2) - 3;
		collider.feet.offsetY = sprite->getHeight() - 8;
		world->emplace<CharacterCollider>(entity, collider);
		world->emplace<SpriteRenderer>(entity, sprite, true, 1);

		createAnimationBlocks(*sprite);
		sprite->setBlock("idle");
	} catch (...) {
		if (world->alive(entity))
			world->destroy(entity);
		throw;
	}
}

Character::~Character() {
	if (world && world->alive(entity))
		world->destroy(entity);
}

Entity Character::getEntity() const noexcept {
	return entity;
}

bool Character::isAlive() const {
	return world && world->alive(entity);
}

void Character::setMoveAxis(double axis) {
	if (auto* controller = getController())
		controller->moveAxis = std::clamp(axis, -1.0, 1.0);
}

double Character::getMoveAxis() const {
	const auto* controller = getController();
	return controller ? controller->moveAxis : 0.0;
}

void Character::setJumpPressed(bool pressed) {
	if (auto* controller = getController())
		controller->jumpPressed = pressed;
}

CharacterController* Character::getController() {
	return world ? world->tryGet<CharacterController>(entity) : nullptr;
}

const CharacterController* Character::getController() const {
	return world ? world->tryGet<CharacterController>(entity) : nullptr;
}

void Character::createAnimationBlocks(Sprite& sprite) {
	SpriteBlock& idle = sprite.createBlock("idle");
	idle.addFrame(SpriteFrame{{2, 4}, {0.9f, 0.1f}});
	idle.addFrame(SpriteFrame{{3, 5}, {0.9f, 0.1f}});
	idle.setSpeed(3.0f);

	SpriteBlock& walk = sprite.createBlock("walk");
	walk.addFrame(SpriteFrame{{0}, {1.0f}});
	walk.addFrame(SpriteFrame{{2}, {1.0f}});
	walk.addFrame(SpriteFrame{{1}, {1.0f}});
	walk.addFrame(SpriteFrame{{2}, {1.0f}});
	walk.setSpeed(5.0f);
}

void Character::registerSystems(World& world, std::vector<Rect<double>>& worldColliders) {
	world.addSystem("character control", [](World& owner) {
		owner.each<CharacterController, Velocity, SpriteRenderer>(
			[](Entity, CharacterController& controller, Velocity& velocity,
				SpriteRenderer& renderer) {
				velocity.x = controller.walkSpeed * controller.moveAxis;
				velocity.y -= controller.jumpSpeed * (controller.jumpPressed ? 1.0 : 0.0);

				if (controller.moveAxis != 0.0)
					renderer.facingDirection = controller.moveAxis < 0.0 ? -1 : 1;
			}
		);
	}, 0);

	world.addSystem("character physics", [&worldColliders](World& owner, float dt) {
		owner.each<Transform, Velocity, CharacterController, CharacterCollider>(
			[&worldColliders, dt](Entity, Transform& transform, Velocity& velocity,
				CharacterController& controller, CharacterCollider& collider) {
				velocity.y = std::min(
					velocity.y + controller.gravity * static_cast<double>(dt),
					controller.terminalVelocity
				);

				const double moveX = velocity.x * static_cast<double>(dt);
				transform.x += moveX;
				Rect<double> box = collider.feet.getBounds(transform.x, transform.y);

				for (const Rect<double>& obstacle : worldColliders) {
					if (!Collider::intersects(box, obstacle))
						continue;

					transform.x = moveX > 0.0
						? obstacle.x - collider.feet.offsetX - collider.feet.w
						: obstacle.x + obstacle.w - collider.feet.offsetX;

					velocity.x = 0.0;
					box = collider.feet.getBounds(transform.x, transform.y);
				}

				const double moveY = velocity.y * static_cast<double>(dt);
				transform.y += moveY;
				box = collider.feet.getBounds(transform.x, transform.y);

				for (const Rect<double>& obstacle : worldColliders) {
					if (!Collider::intersects(box, obstacle))
						continue;

					transform.y = moveY > 0.0
						? obstacle.y - collider.feet.offsetY - collider.feet.h
						: obstacle.y + obstacle.h - collider.feet.offsetY;

					velocity.y = 0.0;
					box = collider.feet.getBounds(transform.x, transform.y);
				}
			}
		);
	}, 10);

	world.addSystem("character animation", [](World& owner, float dt) {
		owner.each<CharacterController, SpriteRenderer>(
			[dt](Entity, CharacterController& controller, SpriteRenderer& renderer) {
				if (!renderer.sprite)
					return;

				renderer.sprite->setBlock(controller.moveAxis == 0.0 ? "idle" : "walk");
				renderer.sprite->step(dt);
			}
		);
	}, 20);
}

} // namespace Tenshi