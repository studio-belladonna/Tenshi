#pragma once

#include "Tenshi/Graphics/Renderer.h"

#include <string>
#include <memory>

namespace Tenshi {

class Font;

class TextRenderer {
public:
	TextRenderer();
	~TextRenderer();

	void Draw();
	void RebuildTexture();
	void SubmitText(const std::string& t);

	void SetPosition(float x, float y);

	void SetColour(rgba col);
	void SetAntialiasing(bool enabled);
	void SetFont(Font* f);
	void SetRenderer(Renderer& renderer);

	void ClearText();

	std::string GetText();
	rgba GetColour();
private:
	Renderer* renderer = nullptr;
	std::unique_ptr<GraphicsTexture> texture;

	Font* font = nullptr;
	
	rgba colour;
	Rect<float> rect;

	std::string text = "";
	
	float x;
	float y;
	float textureOffsetX = 0.0f;
	bool antialiased = false;

	bool dirty = true;
};

std::unique_ptr<TextRenderer> MakeTextRenderer(Renderer& renderer, Font* font);

} // namespace Tenshi