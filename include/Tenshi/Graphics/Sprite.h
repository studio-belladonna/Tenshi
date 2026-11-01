#pragma once

#include <vector>
#include <string>
#include <memory>

#include "Tenshi/Graphics/Renderer.h"
#include "Tenshi/Graphics/SpriteBatch.h"

namespace Tenshi {

class Camera; 

using SpriteImagePosition = int;

// these frames are sub-group of image positions within the sprite texture
// this allows for random sprite variation (like character blinking)
struct SpriteFrame {
    SpriteImagePosition getRandomImage() const;

    std::vector<SpriteImagePosition> images;
    std::vector<float> chance;
};

// grouping of image positions within the sprite texture
class SpriteBlock {
public:
    SpriteBlock(const std::string& h) : handle(h) {}

    const std::string& getHandle() const { return handle; }

    void setSpeed(float speed);
    void setFrame(int f);
    void step(double dt);

    SpriteImagePosition getCurrentImage() const;

    void addFrame(const SpriteFrame& f);
private:
    std::string handle;
    std::vector<SpriteFrame> frames;

    int frame = 0;
    float timer = 0.0f;

    SpriteImagePosition currentImage = 0;

    float animationSpeed = 0.1f;
};

class Sprite {
public:
    Sprite(std::unique_ptr<GraphicsTexture> tex, int frameW, int frameH);
    ~Sprite();

    void setImage(int index);
    void setOrigin(float x, float y);
    void centreOrigin();

    void step(double dt);
    void draw(SpriteBatch& batch, const Camera& cam, int x, int y);

    SpriteBlock& createBlock(const std::string& handle);

    void resetFrame();

    void setBlock(const std::string& handle);
    void setScale(float s);
    void setRotation(float r);
    void setFlip(TextureFlip f);

    GraphicsTexture* GetTexture();
    int getWidth();
    int getHeight();

private:
    std::unique_ptr<GraphicsTexture> texture;

    std::vector<SpriteBlock> blocks;
    int currentBlock = -1;

    Rect<float> src;
    Rect<float> dst;

    int imageWidth;
    int imageHeight;

    int image = 0;

    float originX = 0.0f;
    float originY = 0.0f;

    float scale = 1.0f;
    float rotation = 0.0f;

    TextureFlip flip = TextureFlip::None;
};

} // namespace Tenshi