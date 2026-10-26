#pragma once

#include <vector>
#include <memory>

#include "Tenshi/Scene/Background.h"
#include "Tenshi/Graphics/Tileset.h"
#include "Tenshi/Graphics/Tilemap.h"
#include "Tenshi/Components/Components.h"
#include "Tenshi/Entity/World.h"
#include "Tenshi/Character/Character.h"

namespace Tenshi {

class Game;

class Scene {
public:
	explicit Scene(Game* game);
	~Scene() = default;

	Scene(const Scene&) = delete;
	Scene& operator=(const Scene&) = delete;

	void init();
	void Step(double dt);
	void Draw();

	void setBackground(std::unique_ptr<Background> bg);
	void setTileset(std::unique_ptr<Tileset> ts);
	void setTilemap(std::unique_ptr<Tilemap> map);

	Entity createCharacter(Sprite* sprite, double x = 0.0, double y = 0.0);
	Character* getCharacter(Entity entity);
	const Character* getCharacter(Entity entity) const;

	void setCameraTarget(Entity entity);
	Entity getCameraTarget() const;
	World& getWorld();
	const World& getWorld() const;

	std::unique_ptr<Background>& getBackground();
	std::unique_ptr<Tileset>& getTileset();
	std::unique_ptr<Tilemap>& getTilemap();
	std::vector<Rect<double>>& getColliders();
	std::unique_ptr<Camera>& getCamera();
	
private:
	std::unique_ptr<SpriteBatch> batch;
	std::unique_ptr<Tileset> tileset;
	std::unique_ptr<Tilemap> tilemap;
	std::unique_ptr<Background> background;
	std::vector<Rect<double>> worldColliders;
	std::vector<std::unique_ptr<Character>> characters;
	World world;

	// TODO: make this a vector when scenes support multiple viewports
	std::unique_ptr<Camera> camera;
};

} // namespace Tenshi