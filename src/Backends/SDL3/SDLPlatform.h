#pragma once

#include "Tenshi/Platform/Platform.h"

#include "Tenshi/Input/InputEvent.h"

#include <SDL3/SDL.h>

namespace Tenshi {

class SDLPlatform : public Platform {
public:
    SDLPlatform() = default;
    ~SDLPlatform() override;

    bool initialize() override;
    void shutdown() override;

    double getTime() const override;
    uint64_t getTicks() const override;

    void delay(uint32_t milliseconds) override;
    void pollEvents(const EventCallback& callback) override;

    const char* getBasePath() const override;
private:
    bool translateEvent(const SDL_Event& source, Event& output) const;

    Key translateKey(SDL_Scancode key) const;
    MouseButton translateMouseButton(uint8_t button) const;

    static bool eventWatch(
        void* userdata,
        SDL_Event* event
    );

    EventCallback m_callback;
};



} // namespace Tenshi