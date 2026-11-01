#include "Tenshi/Graphics/Renderer.h"

#include "Tenshi/Core/Log.h"

#include <SDL3/SDL.h>

#include <unordered_set>
#include <utility>

namespace Tenshi {
namespace {

struct SDLGraphicsState {
    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    std::unordered_set<SDL_Texture*> textures;
};

class SDLTexture final : public GraphicsTexture {
public:
    SDLTexture(
        SDL_Texture* texture,
        int width,
        int height,
        std::shared_ptr<SDLGraphicsState> state
    )
        : texture(texture)
        , width(width)
        , height(height)
        , state(std::move(state))
    {
        this->state->textures.insert(texture);
    }

    ~SDLTexture() override {
        if (texture && state->renderer) {
            state->textures.erase(texture);
            SDL_DestroyTexture(texture);
        }
    }

    int getWidth() const override {
        return width;
    }

    int getHeight() const override {
        return height;
    }

    SDL_Texture* getNative() const {
        return texture;
    }

private:
    SDL_Texture* texture;
    int width;
    int height;
    std::shared_ptr<SDLGraphicsState> state;
};

SDL_FRect ToSDLRect(const Rect<float>& rect) {
    return { rect.x, rect.y, rect.w, rect.h };
}

class SDLGraphicsBackend final : public Renderer {
public:
    bool initialize(const RendererConfig& config) override {
        if (state->renderer)
            return true;

        SDL_WindowFlags flags = config.windowResizable
            ? SDL_WINDOW_RESIZABLE
            : 0;
        const int width = config.windowWidth * config.windowScale;
        const int height = config.windowHeight * config.windowScale;

        state->window = SDL_CreateWindow(
            config.title.c_str(),
            width,
            height,
            flags
        );
        if (!state->window) {
            LogError(
                LogCategory::Application,
                "Could not create window: {}",
                SDL_GetError()
            );
            return false;
        }

        state->renderer = SDL_CreateRenderer(state->window, nullptr);
        if (!state->renderer) {
            LogError(
                LogCategory::Renderer,
                "Could not create renderer: {}",
                SDL_GetError()
            );
            shutdown();
            return false;
        }

        SDL_SetRenderVSync(state->renderer, 1);
        setLogicalSize(config.windowWidth, config.windowHeight);
        return true;
    }

    void shutdown() override {
        if (state->renderer) {
            for (SDL_Texture* texture : state->textures)
                SDL_DestroyTexture(texture);
            state->textures.clear();
            SDL_DestroyRenderer(state->renderer);
            state->renderer = nullptr;
        }

        if (state->window) {
            SDL_DestroyWindow(state->window);
            state->window = nullptr;
        }
    }

    std::unique_ptr<GraphicsTexture> createTexture(
        const uint8_t* rgbaPixels,
        int width,
        int height
    ) override {
        if (!state->renderer || !rgbaPixels || width <= 0 || height <= 0)
            return nullptr;

        SDL_Texture* texture = SDL_CreateTexture(
            state->renderer,
            SDL_PIXELFORMAT_RGBA32,
            SDL_TEXTUREACCESS_STATIC,
            width,
            height
        );
        if (!texture) {
            LogError(
                LogCategory::Renderer,
                "Could not create texture: {}",
                SDL_GetError()
            );
            return nullptr;
        }

        if (!SDL_UpdateTexture(texture, nullptr, rgbaPixels, width * 4)) {
            LogError(
                LogCategory::Renderer,
                "Could not upload texture pixels: {}",
                SDL_GetError()
            );
            SDL_DestroyTexture(texture);
            return nullptr;
        }

        SDL_SetTextureScaleMode(texture, SDL_SCALEMODE_NEAREST);
        SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
        return std::make_unique<SDLTexture>(texture, width, height, state);
    }

    void setWindowSize(int width, int height) override {
        if (state->window)
            SDL_SetWindowSize(state->window, width, height);
    }

    void setLogicalSize(int width, int height) override {
        if (state->renderer) {
            SDL_SetRenderLogicalPresentation(
                state->renderer,
                width,
                height,
                SDL_LOGICAL_PRESENTATION_INTEGER_SCALE
            );
        }
    }

    void getOutputSize(int& width, int& height) const override {
        width = 0;
        height = 0;
        if (state->renderer)
            SDL_GetRenderOutputSize(state->renderer, &width, &height);
    }

    void clear(rgba colour) override {
        if (!state->renderer)
            return;

        SDL_SetRenderDrawColor(
            state->renderer,
            static_cast<uint8_t>(colour.r),
            static_cast<uint8_t>(colour.g),
            static_cast<uint8_t>(colour.b),
            static_cast<uint8_t>(colour.a)
        );
        SDL_RenderClear(state->renderer);
    }

    void present() override {
        if (state->renderer)
            SDL_RenderPresent(state->renderer);
    }

    void fillRect(const Rect<float>& rect, rgba colour) override {
        if (!state->renderer)
            return;

        SDL_SetRenderDrawColor(
            state->renderer,
            static_cast<uint8_t>(colour.r),
            static_cast<uint8_t>(colour.g),
            static_cast<uint8_t>(colour.b),
            static_cast<uint8_t>(colour.a)
        );
        const SDL_FRect destination = ToSDLRect(rect);
        SDL_RenderFillRect(state->renderer, &destination);
    }

    void drawTexture(
        const GraphicsTexture& texture,
        const Rect<float>& source,
        const Rect<float>& destination,
        float rotation,
        Vec<float, 2> origin,
        TextureFlip flip
    ) override {
        const auto* sdlTexture = dynamic_cast<const SDLTexture*>(&texture);
        if (!state->renderer || !sdlTexture)
            return;

        const SDL_FRect src = ToSDLRect(source);
        const SDL_FRect dst = ToSDLRect(destination);
        const SDL_FPoint center = { origin.x, origin.y };
        SDL_FlipMode sdlFlip = SDL_FLIP_NONE;
        if (flip == TextureFlip::Horizontal)
            sdlFlip = SDL_FLIP_HORIZONTAL;
        else if (flip == TextureFlip::Vertical)
            sdlFlip = SDL_FLIP_VERTICAL;

        SDL_RenderTextureRotated(
            state->renderer,
            sdlTexture->getNative(),
            &src,
            &dst,
            rotation,
            &center,
            sdlFlip
        );
    }

    void drawTextureTiled(const GraphicsTexture& texture, const Rect<float>& destination) override {
        const auto* sdlTexture = dynamic_cast<const SDLTexture*>(&texture);
        if (!state->renderer || !sdlTexture)
            return;

        const SDL_FRect dst = ToSDLRect(destination);
        SDL_RenderTextureTiled(
            state->renderer,
            sdlTexture->getNative(),
            nullptr,
            1.0f,
            &dst
        );
    }

private:
    std::shared_ptr<SDLGraphicsState> state = std::make_shared<SDLGraphicsState>();
};

} // namespace

std::unique_ptr<Renderer> CreateRenderer() {
    return std::make_unique<SDLGraphicsBackend>();
}

} // namespace Tenshi