#include "SDLPlatform.h"

#include <SDL3/SDL.h>
#include <cstdint>
#include <memory>

#include "Tenshi/Core/Log.h"

namespace Tenshi {

std::unique_ptr<Platform> CreatePlatform() {
    return std::make_unique<SDLPlatform>();
}

SDLPlatform::~SDLPlatform() {
    shutdown();
}

bool SDLPlatform::initialize() {
    SDL_SetHint(SDL_HINT_WINDOWS_ENABLE_MESSAGELOOP, "1");

    if (!SDL_Init(
        SDL_INIT_VIDEO 
    |   SDL_INIT_EVENTS
    )) {
        return false;
    }

    SDL_AddEventWatch(SDLPlatform::eventWatch, this);

    return true;
}

void SDLPlatform::shutdown() {
    SDL_RemoveEventWatch(SDLPlatform::eventWatch, this);
    
    SDL_Quit();
}

void SDLPlatform::delay(uint32_t milliseconds) {
    SDL_Delay(milliseconds);
}

void SDLPlatform::pollEvents(const EventCallback& callback) {
    SDL_Event source;

    while (SDL_PollEvent(&source)) {
        Event event;

        if (translateEvent(source, event)) {
            callback(event);
        }
    }
}

bool SDLPlatform::eventWatch(void* userdata, SDL_Event* source) {
    auto* platform = static_cast<SDLPlatform*>(userdata);

    if (!platform->m_callback) {
        return true;
    }

    Event event;

    if (platform->translateEvent(*source, event)) {
        platform->m_callback(event);
    }

    return true;
}

bool SDLPlatform::translateEvent(const SDL_Event& source, Event& output) const {
    switch (source.type) {
        case SDL_EVENT_QUIT:
            output.type = EventType::Quit;
            return true;

        case SDL_EVENT_WINDOW_RESIZED:
            output.type = EventType::WindowResize;
            output.resize.width = source.window.data1;
            output.resize.height = source.window.data2;
            return true;

        case SDL_EVENT_WINDOW_EXPOSED:
            output.type = EventType::WindowExposed;
            return true;

        case SDL_EVENT_KEY_DOWN:
            output.type = EventType::KeyDown;
            output.input.type = InputEventType::KeyDown;
            output.input.key = translateKey(source.key.scancode);

            return output.input.key != Key::Unknown;

        case SDL_EVENT_KEY_UP:
            output.type = EventType::KeyUp;
            output.input.type = InputEventType::KeyUp;
            output.input.key = translateKey(source.key.scancode);

            return output.input.key != Key::Unknown;

        case SDL_EVENT_MOUSE_BUTTON_DOWN:
            output.type = EventType::MouseButtonDown;
            output.input.type = InputEventType::MouseButtonDown;

            output.input.button = translateMouseButton(source.button.button);

            return output.input.button != MouseButton::Count;

        case SDL_EVENT_MOUSE_BUTTON_UP:
            output.type = EventType::MouseButtonUp;
            output.input.type = InputEventType::MouseButtonUp;

            output.input.button = translateMouseButton(source.button.button);

            return output.input.button != MouseButton::Count;

        case SDL_EVENT_MOUSE_MOTION:
            output.type = EventType::MouseMotion;
            output.input.type = InputEventType::MouseMotion;

            output.input.mouseX = static_cast<int>(source.motion.x);
            output.input.mouseY = static_cast<int>(source.motion.y);

            return true;

        case SDL_EVENT_MOUSE_WHEEL:
            output.type = EventType::MouseWheel;
            output.input.type = InputEventType::MouseWheel;

            output.input.wheelX = static_cast<int>(source.wheel.x);
            output.input.wheelY = static_cast<int>(source.wheel.y);

            return true;

        case SDL_EVENT_TEXT_INPUT:
            output.type = EventType::TextInput;
            output.textInput.text = source.text.text;

            return true;

        case SDL_EVENT_DROP_FILE:
            output.type = EventType::FileDrop;

            if (source.drop.data) {
                output.fileDrop.path = source.drop.data;
            }

            return true;

        default:
            return false;
    }
}

Key SDLPlatform::translateKey(SDL_Scancode key) const {
    switch (key) {
        case SDL_SCANCODE_A: return Key::A;
        case SDL_SCANCODE_B: return Key::B;
        case SDL_SCANCODE_C: return Key::C;
        case SDL_SCANCODE_D: return Key::D;
        case SDL_SCANCODE_E: return Key::E;
        case SDL_SCANCODE_F: return Key::F;
        case SDL_SCANCODE_G: return Key::G;
        case SDL_SCANCODE_H: return Key::H;
        case SDL_SCANCODE_I: return Key::I;
        case SDL_SCANCODE_J: return Key::J;
        case SDL_SCANCODE_K: return Key::K;
        case SDL_SCANCODE_L: return Key::L;
        case SDL_SCANCODE_M: return Key::M;
        case SDL_SCANCODE_N: return Key::N;
        case SDL_SCANCODE_O: return Key::O;
        case SDL_SCANCODE_P: return Key::P;
        case SDL_SCANCODE_Q: return Key::Q;
        case SDL_SCANCODE_R: return Key::R;
        case SDL_SCANCODE_S: return Key::S;
        case SDL_SCANCODE_T: return Key::T;
        case SDL_SCANCODE_U: return Key::U;
        case SDL_SCANCODE_V: return Key::V;
        case SDL_SCANCODE_W: return Key::W;
        case SDL_SCANCODE_X: return Key::X;
        case SDL_SCANCODE_Y: return Key::Y;
        case SDL_SCANCODE_Z: return Key::Z;

        case SDL_SCANCODE_0: return Key::Num0;
        case SDL_SCANCODE_1: return Key::Num1;
        case SDL_SCANCODE_2: return Key::Num2;
        case SDL_SCANCODE_3: return Key::Num3;
        case SDL_SCANCODE_4: return Key::Num4;
        case SDL_SCANCODE_5: return Key::Num5;
        case SDL_SCANCODE_6: return Key::Num6;
        case SDL_SCANCODE_7: return Key::Num7;
        case SDL_SCANCODE_8: return Key::Num8;
        case SDL_SCANCODE_9: return Key::Num9;

        case SDL_SCANCODE_ESCAPE: return Key::Escape;
        case SDL_SCANCODE_RETURN: return Key::Enter;
        case SDL_SCANCODE_TAB: return Key::Tab;
        case SDL_SCANCODE_BACKSPACE: return Key::Backspace;
        case SDL_SCANCODE_SPACE: return Key::Space;

        case SDL_SCANCODE_MINUS: return Key::Minus;
        case SDL_SCANCODE_EQUALS: return Key::Equals;
        case SDL_SCANCODE_LEFTBRACKET: return Key::LeftBracket;
        case SDL_SCANCODE_RIGHTBRACKET: return Key::RightBracket;
        case SDL_SCANCODE_BACKSLASH: return Key::Backslash;
        case SDL_SCANCODE_SEMICOLON: return Key::Semicolon;
        case SDL_SCANCODE_APOSTROPHE: return Key::Apostrophe;
        case SDL_SCANCODE_GRAVE: return Key::Grave;
        case SDL_SCANCODE_COMMA: return Key::Comma;
        case SDL_SCANCODE_PERIOD: return Key::Period;
        case SDL_SCANCODE_SLASH: return Key::Slash;

        case SDL_SCANCODE_CAPSLOCK: return Key::CapsLock;

        case SDL_SCANCODE_LSHIFT: return Key::LeftShift;
        case SDL_SCANCODE_RSHIFT: return Key::RightShift;

        case SDL_SCANCODE_LCTRL: return Key::LeftCtrl;
        case SDL_SCANCODE_RCTRL: return Key::RightCtrl;

        case SDL_SCANCODE_LALT: return Key::LeftAlt;
        case SDL_SCANCODE_RALT: return Key::RightAlt;

        case SDL_SCANCODE_LEFT: return Key::Left;
        case SDL_SCANCODE_RIGHT: return Key::Right;
        case SDL_SCANCODE_UP: return Key::Up;
        case SDL_SCANCODE_DOWN: return Key::Down;

        case SDL_SCANCODE_INSERT: return Key::Insert;
        case SDL_SCANCODE_DELETE: return Key::Delete;
        case SDL_SCANCODE_HOME: return Key::Home;
        case SDL_SCANCODE_END: return Key::End;
        case SDL_SCANCODE_PAGEUP: return Key::PageUp;
        case SDL_SCANCODE_PAGEDOWN: return Key::PageDown;

        case SDL_SCANCODE_PRINTSCREEN: return Key::PrintScreen;
        case SDL_SCANCODE_SCROLLLOCK: return Key::ScrollLock;
        case SDL_SCANCODE_PAUSE: return Key::Pause;

        case SDL_SCANCODE_F1: return Key::F1;
        case SDL_SCANCODE_F2: return Key::F2;
        case SDL_SCANCODE_F3: return Key::F3;
        case SDL_SCANCODE_F4: return Key::F4;
        case SDL_SCANCODE_F5: return Key::F5;
        case SDL_SCANCODE_F6: return Key::F6;
        case SDL_SCANCODE_F7: return Key::F7;
        case SDL_SCANCODE_F8: return Key::F8;
        case SDL_SCANCODE_F9: return Key::F9;
        case SDL_SCANCODE_F10: return Key::F10;
        case SDL_SCANCODE_F11: return Key::F11;
        case SDL_SCANCODE_F12: return Key::F12;

        case SDL_SCANCODE_KP_0: return Key::Numpad0;
        case SDL_SCANCODE_KP_1: return Key::Numpad1;
        case SDL_SCANCODE_KP_2: return Key::Numpad2;
        case SDL_SCANCODE_KP_3: return Key::Numpad3;
        case SDL_SCANCODE_KP_4: return Key::Numpad4;
        case SDL_SCANCODE_KP_5: return Key::Numpad5;
        case SDL_SCANCODE_KP_6: return Key::Numpad6;
        case SDL_SCANCODE_KP_7: return Key::Numpad7;
        case SDL_SCANCODE_KP_8: return Key::Numpad8;
        case SDL_SCANCODE_KP_9: return Key::Numpad9;

        case SDL_SCANCODE_KP_DIVIDE:
            return Key::NumpadDivide;

        case SDL_SCANCODE_KP_MULTIPLY:
            return Key::NumpadMultiply;

        case SDL_SCANCODE_KP_MINUS:
            return Key::NumpadMinus;

        case SDL_SCANCODE_KP_PLUS:
            return Key::NumpadPlus;

        case SDL_SCANCODE_KP_ENTER:
            return Key::NumpadEnter;

        case SDL_SCANCODE_KP_PERIOD:
            return Key::NumpadPeriod;

        default:
            return Key::Unknown;
    }
}

MouseButton SDLPlatform::translateMouseButton(uint8_t button) const {
    switch (button) {
        case SDL_BUTTON_LEFT:
            return MouseButton::Left;

        case SDL_BUTTON_MIDDLE:
            return MouseButton::Middle;

        case SDL_BUTTON_RIGHT:
            return MouseButton::Right;

        case SDL_BUTTON_X1:
            return MouseButton::X1;

        case SDL_BUTTON_X2:
            return MouseButton::X2;

        default:
            return MouseButton::Count;
    }
}

double SDLPlatform::getTime() const {
    return  static_cast<double>(SDL_GetPerformanceCounter()) 
        /   static_cast<double>(SDL_GetPerformanceFrequency());
}

uint64_t SDLPlatform::getTicks() const {
    return SDL_GetTicks();
}

const char* SDLPlatform::getBasePath() const {
    const char* basePath = SDL_GetBasePath();

    if (!basePath) {
        LogError(
            LogCategory::Application,
            "Could not determine application path"
        );
    }

    return basePath;
}

void SDLPlatform::showError(const char* title, const char* message) {
    SDL_ShowSimpleMessageBox(
        SDL_MESSAGEBOX_ERROR,
        title,
        message,
        nullptr
    );
}

} // namespace Tenshi