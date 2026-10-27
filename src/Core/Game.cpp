#include "Tenshi/Core/Game.h"

#include "Tenshi/Files/Yaml.h"

#include "Platform/Window.h"
#include "Core/GameHelper.h"
#include "Core/CrashHandler.h"

#include <utility>
#include <string>
#include <cstdint>
#include <filesystem>

#undef min
#undef max
#include <algorithm>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

namespace Tenshi {

#ifdef __EMSCRIPTEN__
Game* Game::emscriptenInstance = nullptr;

void Game::emscriptenStep() {
    if (emscriptenInstance) {
        emscriptenInstance->step();
    }
}
#endif

Game::Game(std::unique_ptr<Platform> platform)
    : platform(std::move(platform)) 
{
}

Game::~Game() {
    subroutines.clear();
    audiosys.Shutdown();

    if (renderer) {
        SDL_DestroyRenderer(renderer);
        renderer = nullptr;
    }

    if (window) {
        SDL_DestroyWindow(window);
        window = nullptr;
    }

    if (platform) {
        platform->shutdown();
    }
}

bool Game::init() {
    if (!platform) {
        FatalError(
            "Error!",
            "No platform backend was provided"
        );

        return false;
    }

    if (!platform->initialize()) {
        FatalError(
            "Error!",
            "Platform initialization failed"
        );

        return false;
    }

    const char* basePath = platform->getBasePath();
    std::filesystem::path root(basePath);
    filesys.setRoot(root.string());

    bool mountedPak = false;
    for (uint64_t chunk = 0;; ++chunk) {
        const std::string pakName = "resource"
            + std::to_string(chunk) + ".tpk";

        if (!filesys.mountPak("", pakName)) {
            break;
        }

        mountedPak = true;
    }

    if (!mountedPak) {
        filesys.mountDirectory("", "resources");
    }

    YamlLoader yaml(filesys);
    YamlDocument document = yaml.Load("game.yml");
    INTERNAL::buildConfig(document, config);

    window = MakeWindow(config);
    if (!window) return false;

    renderer = SDL_CreateRenderer(window, NULL);
    if (!renderer) return false;

    SDL_SetRenderVSync(renderer, 1);

    resetRenderer(config);

    if (!audiosys.Init()) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Audio engine failed to initialize");
        return false;
    }

#ifndef __EMSCRIPTEN__
    if (!audiosys.Start()) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Audio device failed to start");
        return false;
    }
#endif

    audiosys.SetMasterVolume(1.0f);

    lastTime = platform->getTime();
    deltaTime = 0.0;
    targetFrameTime = 1.0 / 60.0;

    running = true;
    return true;
}

void Game::run() {
#ifdef __EMSCRIPTEN__
    emscriptenInstance = this;

    emscripten_set_main_loop(
        Game::emscriptenStep,
        0,
        true
    );
#else
    while (running) {
        step();
    }
#endif
}

void Game::quit() {
    running = false;

#ifdef __EMSCRIPTEN__
    emscripten_cancel_main_loop();
#endif
}

void Game::addSubroutine(std::unique_ptr<Subroutine> subroutine) {
    if (!subroutine) return;

    subroutines.push_back(std::move(subroutine));
}

void Game::step() {
	const double currentTime = platform->getTime();
	double elapsed = currentTime - lastTime;
	lastTime = currentTime;

	if (elapsed < 0.0) {
		elapsed = 0.0;
	}

	frameTime = std::min(elapsed, maxFrameTime);
	deltaTime = frameTime;
    
    input.beginFrame();

    platform->pollEvents(
        [this](const Event& event) {
            processEvent(event);
        }
    );

    physicsAccumulator += frameTime;
	int physicsSteps = 0;

    while (physicsAccumulator >= physicsTimeStep
        && physicsSteps < maxPhysicsSteps
    ) {
        // signal all physics
		for (auto& subroutine : subroutines) {
			subroutine->fixedUpdate(physicsTimeStep);
		}

		physicsAccumulator -= physicsTimeStep;
		physicsSteps++;
	}

	if (physicsSteps == maxPhysicsSteps
    &&  physicsAccumulator >= physicsTimeStep
    ) {
		physicsAccumulator = 0.0;
	}

    // signal all updates
    for (auto& subroutine : subroutines) {
        subroutine->update(frameTime);
    }

    draw();

    redrawRequested = false;
}

void Game::processEvent(const Event& event) {
    switch (event.type) {
        case EventType::Quit:
            running = false;
            break;

        case EventType::WindowResize:
            emit("windowResize");
            break;

        case EventType::WindowExposed:
            redrawRequested = true;
            break;

        default:
            break;
    }

    switch (event.type) {
        case EventType::KeyDown:
        case EventType::KeyUp:
        case EventType::MouseButtonDown:
        case EventType::MouseButtonUp:
        case EventType::MouseMotion:
        case EventType::MouseWheel:
            input.process(event.input);
            break;

        default:
            break;
    }

#ifdef __EMSCRIPTEN__
    // try to start audio in browser
    switch (event.type) {
        case EventType::KeyDown:
        case EventType::MouseButtonDown:
        case EventType::MouseMotion:
        case EventType::MouseWheel:
            audiosys.Start();
            break;

        default:
            break;
    }
#endif

    // signal all events
    for (auto& subroutine : subroutines) {
        subroutine->event(event);
    }
}

void Game::draw() {
    if (config.clearFrame) {
        SDL_SetRenderDrawColor(renderer, 
            config.clearColour.r,
            config.clearColour.g,
            config.clearColour.b,
            config.clearColour.a
        );

        SDL_RenderClear(renderer);
    }

    // signal all drawing
    for (auto& subroutine : subroutines) {
        subroutine->draw();
    }

    SDL_RenderPresent(renderer);
}

void Game::pulse() {
    if (redrawRequested) {
        draw();

        redrawRequested = false;
    }
}

GameConfig& Game::getConfig() {
    return config;
}

Input& Game::getInput() {
    return input;
}

void Game::resetRenderer(GameConfig conf) {
    SDL_SetWindowSize(window,
        conf.windowWidth  * conf.windowScale,
        conf.windowHeight * conf.windowScale
    );

    rendererSetFixedSize(conf.windowWidth, conf.windowHeight);
}

void Game::rendererSetFixedSize(int w, int h) {
    SDL_SetRenderLogicalPresentation(renderer, w, h, SDL_LOGICAL_PRESENTATION_INTEGER_SCALE);
}

FileSystem& Game::getFileSystem() {
    return filesys;
}

AudioSystem& Game::getAudioSystem() {
    return audiosys;
}

SDL_Window* Game::getWindow() {
    return window;
}

SDL_Renderer* Game::getRenderer() {
    return renderer;
}

} // namespace Tenshi