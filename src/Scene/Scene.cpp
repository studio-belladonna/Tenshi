#include "Tenshi/Scene/Scene.h"

#include "Tenshi/Core/Game.h"

#include <stdexcept>
#include <algorithm>

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
	updateTilemapColliders();
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
	collisionMap = nullptr;
}

void Scene::setTilemap(std::unique_ptr<Tilemap> map) {
	tilemap = std::move(map);
	if (tilemap)
		tilemap->SetTileset(tileset.get());
	collisionMap = nullptr;
}

void Scene::updateTilemapColliders() {
	const Tilemap* map = tilemap.get();
	const int tileWidth = tileset ? tileset->GetTileWidth() : 16;
	const int tileHeight = tileset ? tileset->GetTileHeight() : 16;
	const uint64_t revision = map ? map->GetCollisionRevision() : 0;

	worldColliders = colliders;
	if (map == collisionMap
	&& 	revision == collisionRevision
	&& 	tileWidth == collisionTileWidth
	&& 	tileHeight == collisionTileHeight
	) {
		worldColliders.insert(
			worldColliders.end(),
			tilemapColliders.begin(),
			tilemapColliders.end()
		);
		return;
	}

	tilemapColliders.clear();
	collisionMap = map;
	collisionRevision = revision;
	collisionTileWidth = tileWidth;
	collisionTileHeight = tileHeight;

	if (!map || tileWidth <= 0 || tileHeight <= 0)
		return;

	for (int y = 0; y < map->GetHeight(); ++y) {
		for (int x = 0; x < map->GetWidth(); ++x) {
			const auto type = static_cast<TileColliderType>(
				map->GetCollision(x, y)
			);
			if (type == TileColliderType::NONE)
				continue;

			double colliderY = static_cast<double>(y * tileHeight);
			double colliderHeight = static_cast<double>(tileHeight);
			if (type == TileColliderType::HALF) {
				colliderHeight = std::max(1, tileHeight / 2);
				colliderY += tileHeight - colliderHeight;
			}

			tilemapColliders.push_back({
				static_cast<double>(x * tileWidth),
				colliderY,
				static_cast<double>(tileWidth),
				colliderHeight
			});
		}
	}

	worldColliders.insert(
		worldColliders.end(),
		tilemapColliders.begin(),
		tilemapColliders.end()
	);
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
	return colliders;
}

std::unique_ptr<Camera>& Scene::getCamera() {
	return camera;
}

} // namespace Tenshi
