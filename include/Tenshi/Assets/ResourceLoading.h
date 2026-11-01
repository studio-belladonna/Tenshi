#pragma once

#include <string>

#include "Tenshi/Assets/AssetManager.h"
#include "Tenshi/Graphics/Renderer.h"

namespace Tenshi {

class FileSystem;

bool MakeSprite(
    Renderer& renderer,
    AssetManager& assets,
    FileSystem& files,
    const std::string& path,
    const std::string& key,
    int width,
    int height
);

bool MakeFont(
	AssetManager& assets, 
	FileSystem& files,
	const std::string path, 
	const std::string key, 
	float ptsize
);

bool MakeSound(
    AssetManager& assets,
    FileSystem& files,
    AudioSystem& audio,
    const std::string& path,
    const std::string& key
);

} // namespace Tenshi