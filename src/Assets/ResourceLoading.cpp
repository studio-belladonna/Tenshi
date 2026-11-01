#include "Tenshi/Assets/ResourceLoading.h"

#include <memory>

#include "Tenshi/Core/Log.h"
#include "Tenshi/Files/FileSystem.h"
#include "Tenshi/Audio/Audio.h"
#include "Tenshi/Graphics/Sprite.h"
#include "Tenshi/Audio/Sound.h"

#include "Tenshi/Assets/Font.h"
#include "Assets/Image.h"

#include "Files/PakFile.h"

namespace Tenshi {

// Create a new sprite and put it into an AssetManager
bool MakeSprite(
    Renderer& renderer,
    AssetManager& assets,
    FileSystem& files,
    const std::string& path,
    const std::string& key,
    int width,
    int height
) {
    auto data = files.readAll(path);
    if (data.empty()) {
        LogWarning(
            Tenshi::LogCategory::Resource, 
            "Could not find image at: {}", 
            path
        );
        return false;
    }

    auto texture = LoadTextureSTB(renderer, data.data(), data.size());
    if (!texture) {
        LogDebug(
            LogCategory::Resource,
            "Failed to load sprite '{}'",
            path
        );

        return false;
    }

    auto sprite = std::make_unique<Sprite>(std::move(texture), width, height);
    assets.sprite.Add(key, std::move(sprite));

    return true;
}

// Create a new font and put it into an AssetManager
bool MakeFont(
	AssetManager& assets, 
	FileSystem& files,
	const std::string path, 
	const std::string key, 
	float ptsize
) {
	auto data = files.readAll(path);
    if (data.empty()) return false;

	auto font = std::make_unique<Font>();
	if (!font->LoadFromMemory(std::move(data), ptsize)) {
		LogDebug(
			LogCategory::Application,
			"Failed to load font: {}",
			path
		);
		return false;
	}

	assets.font.Add(key, std::move(font));

	return true;
}

// Create a new sound and put it into an AssetManager
bool MakeSound(
    AssetManager& assets,
    FileSystem& files,
    AudioSystem& audio,
    const std::string& path,
    const std::string& key
) {
    auto data = files.readAll(path);
    if (data.empty()) {
        LogWarning(
            LogCategory::Resource,
            "Could not find audio file at: {}",
            path
        );

        return false;
    }

    auto sound = std::make_unique<Sound>();
    if (!sound->Load(audio, std::move(data))) {
        LogWarning(
            LogCategory::Resource,
            "Found but failed to load audio file: {}",
            path
        );

        return false;
    }

    assets.sound.Add(key, std::move(sound));

    return true;
}

} // namespace Tenshi