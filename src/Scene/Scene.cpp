#include "Tenshi/Scene/Scene.h"

#include "Tenshi/Core/Game.h"

#include <stdexcept>

namespace Tenshi {

Scene::Scene(Game* game) {
	if (!game)
		throw std::invalid_argument("Scene requires a valid Game");

	batch = std::make_unique<SpriteBatch>(game->getRenderer());
	camera = std::make_unique<Camera>();

	Character::registerSystems(world, worldColliders);
}

void Scene::init() {
}

void Scene::Step(double dt) {
	world.update(static_cast<float>(dt));

	const auto targets = world.view<CameraTarget>().entities();
	if (!targets.empty()) {
		const Entity target = targets.front();
		const auto* transform = world.tryGet<Transform>(target);
		const auto* velocity = world.tryGet<Velocity>(target);
		const auto* renderer = world.tryGet<SpriteRenderer>(target);

		if (transform && velocity && renderer && renderer->sprite) {
			camera->update(
				dt,
				transform->x + renderer->sprite->getWidth() / 2.0,
				transform->y + renderer->sprite->getHeight() / 2.0,
				velocity->x,
				velocity->y
			);
		} else {
			camera->update(dt);
		}
	} else {
		camera->update(dt);
	}

	if (background)
		background->Step(*camera);
}

void Scene::Draw() {
	if (background)
		background->Draw(*camera);

	if (tilemap)
		tilemap->Draw(*camera);

	batch->begin();

	world.each<Transform, SpriteRenderer>(
		[this](Entity, Transform& transform, SpriteRenderer& renderer) {
			if (!renderer.visible || !renderer.sprite)
				return;

			renderer.sprite->setFlip(
				renderer.facingDirection < 0
					? SDL_FLIP_HORIZONTAL
					: SDL_FLIP_NONE
			);

			renderer.sprite->draw(
				*batch,
				*camera,
				static_cast<int>(transform.x),
				static_cast<int>(transform.y)
			);
		}
	);

	batch->flush();
}

void Scene::setBackground(std::unique_ptr<Background> bg) {
	background = std::move(bg);
}

void Scene::setTileset(std::unique_ptr<Tileset> ts) {
	tileset = std::move(ts);
	if (tilemap)
		tilemap->SetTileset(tileset.get());
}

void Scene::setTilemap(std::unique_ptr<Tilemap> map) {
	tilemap = std::move(map);
	if (tilemap)
		tilemap->SetTileset(tileset.get());
}

Entity Scene::createCharacter(Sprite* sprite, double x, double y) {
	auto character = std::make_unique<Character>(world, sprite, x, y);
	const Entity entity = character->getEntity();
	characters.push_back(std::move(character));
	return entity;
}

Character* Scene::getCharacter(Entity entity) {
	for (const auto& character : characters) {
		if (character->getEntity() == entity && character->isAlive())
			return character.get();
	}
	return nullptr;
}

const Character* Scene::getCharacter(Entity entity) const {
	for (const auto& character : characters) {
		if (character->getEntity() == entity && character->isAlive())
			return character.get();
	}
	return nullptr;
}

void Scene::setCameraTarget(Entity entity) {
	if (!world.alive(entity))
		throw std::invalid_argument("Camera target must be a live entity");
	
	const auto targets = world.view<CameraTarget>().entities();
	for (Entity target : targets) {
		if (target != entity)
			world.remove<CameraTarget>(target);
	}

	if (!world.has<CameraTarget>(entity))
		world.emplace<CameraTarget>(entity);
}

Entity Scene::getCameraTarget() const {
	const auto targets = world.view<CameraTarget>().entities();
	return targets.empty() ? Entity{} : targets.front();
}

World& Scene::getWorld() {
	return world;
}

const World& Scene::getWorld() const {
	return world;
}

std::unique_ptr<Background>& Scene::getBackground() {
	return background;
}

std::unique_ptr<Tileset>& Scene::getTileset() {
	return tileset;
}

std::unique_ptr<Tilemap>& Scene::getTilemap() {
	return tilemap;
}

std::vector<Rect<double>>& Scene::getColliders() {
	return worldColliders;
}

std::unique_ptr<Camera>& Scene::getCamera() {
	return camera;
}

} // namespace Tenshi
