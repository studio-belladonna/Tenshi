#pragma once

#include <string>
#include <vector>
#include <functional>
#include <unordered_map>
#include <iostream>
#include <any>
#include <algorithm>
#include <filesystem>
#include <memory>
#include <cstdint>

#include <SDL3/SDL.h>

#include "Tenshi/Input/Input.h"
#include "Tenshi/Platform/Platform.h"

#include "Tenshi/Files/FileSystem.h"
#include "Tenshi/Core/Subroutine.h"
#include "Tenshi/Audio/Audio.h"

#include "Tenshi/Utility/Types.h"

namespace Tenshi {

struct GameConfig {
    std::string title = "TENSHI Game";

    int windowWidth = 256;
    int windowHeight = 240;

    int windowScale = 1;

    bool windowResizable = false;

    bool clearFrame = true;
    rgba clearColour = {0, 0, 0, 255};
};

class Game {
public:
    Game(std::unique_ptr<Platform> platform);
    ~Game();

    Game(const Game&) = delete;
    Game& operator=(const Game&) = delete;

    // Initialize the game
    bool init();

    // Start the game loop
    void run();

    // End the game loop
    void quit();

    void resetRenderer(GameConfig conf);
    void rendererSetFixedSize(int w, int h);

    void addSubroutine(std::unique_ptr<Subroutine> subroutine);

    FileSystem& getFileSystem();
    AudioSystem& getAudioSystem();
    GameConfig& getConfig();
    Input& getInput();
    SDL_Window* getWindow();
    SDL_Renderer* getRenderer();

    // Register a callback event
    template<typename Callable>
    void on(const std::string& eventName, Callable&& callback) {
        auto func = std::function(
            std::forward<Callable>(callback)
        );
        listeners[eventName].push_back(std::any(func));
    }

    // emit a callback event
    template<typename... Args>
    void emit(const std::string& eventName, Args&&... args) {
        auto it = listeners.find(eventName);
        if (it == listeners.end()) return;

        for (const auto& anyCallback : it->second) {
            using FuncType = std::function<void(Args...)>;
            try {
                auto callback = std::any_cast<FuncType>(anyCallback);
                callback(std::forward<Args>(args)...);
            } catch (const std::bad_any_cast&) {
                std::cerr << "Error: Event signature mismatch for " << eventName << "\n";
            }
        }
    }
public:
    double getDeltaTime() const { 
        return deltaTime; 
    }

    uint32_t getDeltaTimeMs() const { 
        return static_cast<uint32_t>(deltaTime * 1000.0); 
    }

    uint32_t countFPS() const {
        uint32_t now;
        static bool first = true;
        static uint32_t max_count;
        static uint32_t count;
        static uint32_t wait;

        if (first) {
            wait = platform->getTicks();
            first = false;
        }

        now = platform->getTicks();
        count++;

        if (wait + 1000 <= now) {
            wait += 1000;
            max_count = count;
            count = 0;
        }

        return max_count;
    }
private:
    void step();
    void draw();

    // process a platform event
    void processEvent(const Event& event);

    // handle unhandled frame logic
    void pulse();

private:
    std::unique_ptr<Platform> platform;

    Input input;

    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    SDL_Surface* applicationSurface = nullptr;
    GameConfig config;

    FileSystem filesys;
    AudioSystem audiosys;

    double targetFrameTime = 1.0 / 60.0;
    double lastTime = 0.0;
    double deltaTime = 0.0;
    double frameTime = 0.0;
    double physicsAccumulator = 0.0;

    static constexpr double physicsTimeStep = 1.0 / 60.0;
    static constexpr double maxFrameTime = 0.05;
    static constexpr int maxPhysicsSteps = 3;

    bool running = false;
    bool redrawRequested = false;

#ifdef __EMSCRIPTEN__
    static Game* emscriptenInstance;
    static void emscriptenStep();
#endif

    // callbacks
    std::unordered_map<std::string, std::vector<std::any>> listeners;
    std::vector<std::unique_ptr<Subroutine>> subroutines;
};

} // namespace Tenshi