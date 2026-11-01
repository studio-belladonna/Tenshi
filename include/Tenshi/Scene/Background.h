#pragma once

#include "Tenshi/Graphics/Renderer.h"
#include "Tenshi/Graphics/Sprite.h"
#include "Tenshi/Scene/Camera.h"

#include <vector>
#include <stdint.h>
#include <string>
#include <memory>

namespace Tenshi {

class BackgroundLayer {
public:
	BackgroundLayer(Sprite* spr, std::string hndl);
	~BackgroundLayer();

	void Step();
	void Position(double x, double y);

	void SetDepth(uint16_t d);
	void SetSpeed(float speedX, float speedY);

	double GetX() const;
	double GetY() const;

	Sprite* GetSprite();
	uint16_t GetDepth();
	float GetSpeedX();
	float GetSpeedY();

	std::string handle;

	bool horizontalTile, verticalTile;
private:
	Sprite* sprite;

	uint16_t depth; // parallax depth, also used to sort draw order.

	double x, y;

	float scrollSpeedX, scrollSpeedY;
};

class Background {
public:
	explicit Background(Renderer& renderer);
	~Background();

	void Step(const Camera& cam);
	void Draw(const Camera& cam);

	void AddLayer(std::unique_ptr<BackgroundLayer> layer);
	void RemoveLayer(std::string handle);
	void SortLayersDepth();

	bool ParallaxEnabled;
private:
	Renderer& renderer;

	std::vector<std::unique_ptr<BackgroundLayer>> layers;
};

} // namespace Tenshi