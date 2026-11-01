#include "Tenshi/Scene/Background.h"

#include <cmath>

namespace Tenshi {

// layer
BackgroundLayer::BackgroundLayer(Sprite* spr, std::string hndl) {
	sprite = spr;

	handle = hndl;

	x = 0;
	y = 0;

	horizontalTile = true;
	verticalTile   = true;

	depth = 0;
}

BackgroundLayer::~BackgroundLayer() {

}

void BackgroundLayer::Step() {
	x += scrollSpeedX;
	y += scrollSpeedY;
}

void BackgroundLayer::Position(double xx, double yy) {
	x = xx;
	y = yy;
}

void BackgroundLayer::SetDepth(uint16_t d) {
	depth = d;
}

void BackgroundLayer::SetSpeed(float speedX, float speedY) {
	scrollSpeedX = speedX;
	scrollSpeedY = speedY;
}

double BackgroundLayer::GetX() const {
	return x;
}

double BackgroundLayer::GetY() const {
	return y;
}

Sprite* BackgroundLayer::GetSprite() {
	return sprite;
}

uint16_t BackgroundLayer::GetDepth() {
	return depth;
}

float BackgroundLayer::GetSpeedX() {
	return scrollSpeedX;
}

float BackgroundLayer::GetSpeedY() {
	return scrollSpeedY;
}


// background
Background::Background(Renderer& renderer)
	: renderer(renderer)
{
	ParallaxEnabled = true;
}

Background::~Background() {

}

void Background::Step(const Camera& cam) {
	for (auto& layer : layers) {
		layer->Step();

		float factor = 1.0f / std::max(1.0f, (float)layer->GetDepth());
		float px = cam.getViewX() * factor;
		float py = cam.getViewY() * factor;

		layer->Position(px, py);
	}
}

void Background::Draw(const Camera& cam) {
	int screenW, screenH;
	renderer.getOutputSize(screenW, screenH);

	for (const auto& layer : layers) {
		Sprite* spr = layer->GetSprite();
		GraphicsTexture* tex = spr->GetTexture();
		if (!tex)
			continue;

		const float texW = static_cast<float>(tex->getWidth());
		const float texH = static_cast<float>(tex->getHeight());
		if (texW <= 0.0f || texH <= 0.0f)
			continue;

		// parallax
		float depth = std::max(1.0f, (float)layer->GetDepth());
		float factor = 1.0f / depth;

		float camX = cam.getViewX() * factor;
		float camY = cam.getViewY() * factor;

		// wrap offsets to texture size
		float offsetX = fmod(camX, texW);
		float offsetY = fmod(camY, texH);

		if (offsetX < 0) offsetX += texW;
		if (offsetY < 0) offsetY += texH;

		Rect<float> destination{
			-offsetX,
			-offsetY,
			static_cast<float>(screenW) + texW,
			static_cast<float>(screenH) + texH
		};
		renderer.drawTextureTiled(*tex, destination);
	}
}

void Background::AddLayer(std::unique_ptr<BackgroundLayer> layer) {
	layers.push_back(std::move(layer));
}

void Background::RemoveLayer(std::string handle) {
	layers.erase(
		std::remove_if(layers.begin(), layers.end(),
			[&handle](const std::unique_ptr<BackgroundLayer>& layer) {
				return layer->handle == handle;
			}),
		layers.end()
	);
}

void Background::SortLayersDepth() {
	std::sort(layers.begin(), layers.end(), [](std::unique_ptr<BackgroundLayer>& a, std::unique_ptr<BackgroundLayer>& b) {
		return a->GetDepth() > b->GetDepth();
	});
}

} // namespace Tenshi