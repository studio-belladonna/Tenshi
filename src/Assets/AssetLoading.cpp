#include "Tenshi/Assets/AssetLoading.h"

#include "Tenshi/Files/FileSystem.h"
#include "Tenshi/Assets/AssetManager.h"
#include "Tenshi/Assets/ResourceLoading.h"
#include "Tenshi/Files/Yaml.h"

#include <string>
#include <filesystem>
#include <vector>

namespace Tenshi {
namespace {

std::vector<std::string> LoadYamlDir(
    FileSystem& files,
    const std::string& dirpath
) {
    std::vector<std::string> yaml_files;

    for (const auto& path : files.listFiles(dirpath)) {
        const std::filesystem::path filePath(path);
        const std::string extension = filePath.extension().generic_string();
        if (extension != ".yml" && extension != ".yaml") {
            continue;
        }
        
        yaml_files.push_back(path);
    }

    return yaml_files;
}

} // namespace 

bool MakeAllAssetsYaml(
    Renderer& renderer,
    AssetManager& assets,
    FileSystem& files,
    AudioSystem& audio
) {
    YamlLoader yaml(files);
    YamlDocument document = yaml.Load(assets.getYamlPath());

    const auto& yamlRoot = document.GetRoot();

    std::string sprite_path
    =   yamlRoot["asset-definitions"]["sprite"]["path"].get_value<std::string>();

    auto sprite_list = LoadYamlDir(files, sprite_path);
    for (const auto& path : sprite_list) {
        MakeSpriteYaml(renderer, assets, files, path);
    }

    std::string sound_path
    =   yamlRoot["asset-definitions"]["sound"]["path"].get_value<std::string>();

    auto sound_list = LoadYamlDir(files, sound_path);
    for (const auto& path : sound_list) {
        MakeSoundYaml(assets, files, audio, path);
    }

    return true;
}

bool MakeSpriteYaml(
    Renderer& renderer,
    AssetManager& assets,
    FileSystem& files,
    const std::string& path
) {
    YamlLoader yaml(files);
    YamlDocument document = yaml.Load(path);
    
    const auto& yamlRoot = document.GetRoot();

    std::string definition_id
    =   yamlRoot["sprite-definition"]["id"].get_value<std::string>();

    int definition_width 
    =   yamlRoot["sprite-definition"]["width"].get_value<int>();

    int definition_height 
    =   yamlRoot["sprite-definition"]["height"].get_value<int>();

    std::string definition_path
    =   yamlRoot["sprite-definition"]["file"].get_value<std::string>();

    MakeSprite(renderer, assets, files,
        definition_path, 
        definition_id, 
        definition_width, 
        definition_height
    );

    return true;
}

bool MakeSoundYaml(
    AssetManager& assets,
    FileSystem& files,
    AudioSystem& audio,
    const std::string& path
) {
    YamlLoader yaml(files);
    YamlDocument document = yaml.Load(path);
    
    const auto& yamlRoot = document.GetRoot();

    std::string definition_id
    =   yamlRoot["sound-definition"]["id"].get_value<std::string>();

    std::string definition_path
    =   yamlRoot["sound-definition"]["file"].get_value<std::string>();

    MakeSound(assets, files, audio, definition_path, definition_id);

    return true;
}

} // namespace Tenshi