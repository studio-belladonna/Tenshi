#pragma once

#include <cstdint>
#include <vector>

#include "Tenshi/Components/Components.h"
#include "Tenshi/Entity/World.h"
#include "Tenshi/Graphics/Sprite.h"

namespace Tenshi {

class Character {
public:
	Character(World& world, Sprite* sprite, double x = 0.0, double y = 0.0);
	~Character();

	Character(const Character&) = delete;
	Character& operator=(const Character&) = delete;
	Character(Character&&) = delete;
	Character& operator=(Character&&) = delete;

	Entity getEntity() const noexcept;
	bool isAlive() const;

	void setMoveAxis(double axis);
	double getMoveAxis() const;
	void setJumpPressed(bool pressed);

	CharacterController* getController();
	const CharacterController* getController() const;

	static void registerSystems(World& world, std::vector<Rect<double>>& worldColliders);

private:
	static void createAnimationBlocks(Sprite& sprite);

	World* world = nullptr;
	Entity entity{};
};

} // namespace Tenshi