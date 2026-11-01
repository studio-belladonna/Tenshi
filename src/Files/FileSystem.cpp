#include "Tenshi/Files/FileSystem.h"

#include "Files/PakArchive.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <limits>
#include <optional>

namespace fs = std::filesystem;

namespace {
std::optional<std::string> relativePathForMount(const std::string& virtualPath, const std::string& requestedPath) {
    if (virtualPath.empty()) {
        return requestedPath;
    }

    if (requestedPath == virtualPath) {
        return std::string{};
    }

    const std::string prefix = virtualPath + "/";
    if (requestedPath.rfind(prefix, 0) != 0) {
        return std::nullopt;
    }

    return requestedPath.substr(prefix.size());
}
} // namespace

namespace Tenshi {

class DirectoryFile : public File {
public:
    explicit DirectoryFile(const fs::path& path)
        : file(path, std::ios::binary), fileSize(-1)
    {
        if (file) {
            std::error_code error;
            auto size = fs::file_size(path, error);

            if (!error && size <= static_cast<uintmax_t>(std::numeric_limits<int64_t>::max())) {
                fileSize = static_cast<int64_t>(size);
            }
        }
    }

    bool isOpen() const {
        return file.is_open();
    }

    size_t read(void* buffer, size_t size) override {
        if (!file || !buffer || size == 0) {
            return 0;
        }

        const size_t maxStreamSize = static_cast<size_t>(std::numeric_limits<std::streamsize>::max());
        if (size > maxStreamSize) {
            size = maxStreamSize;
        }

        file.read(
            static_cast<char*>(buffer),
            static_cast<std::streamsize>(size)
        );

        return static_cast<size_t>(file.gcount());
    }

    bool seek(int64_t offset, int origin) override {
        if (!file) {
            return false;
        }

        std::ios_base::seekdir direction;

        switch (origin) {
            case SEEK_SET:
                direction = std::ios::beg;
                break;

            case SEEK_CUR:
                direction = std::ios::cur;
                break;

            case SEEK_END:
                direction = std::ios::end;
                break;

            default:
                return false;
        }

        file.clear();
        file.seekg(static_cast<std::streamoff>(offset), direction);

        return !file.fail();
    }

    int64_t tell() const override {
        if (!file) {
            return -1;
        }

        auto position = file.tellg();

        if (position == std::streampos(-1)) {
            return -1;
        }

        return static_cast<int64_t>(position);
    }

    int64_t size() const override {
        return fileSize;
    }

private:
    mutable std::ifstream file;
    int64_t fileSize;
};

struct FileSystem::Mount {
    enum class Type {
        Directory,
        Pak
    };

    Type type;

    std::string virtualPath;
    std::string directory;

    std::shared_ptr<PakArchive> pak;
};

FileSystem::FileSystem() {
}

FileSystem::~FileSystem() = default;

void FileSystem::setRoot(const std::string& path) {
    root = fs::absolute(path).lexically_normal().string();
}

bool FileSystem::mountDirectory(
    const std::string& virtualPath,
    const std::string& directory
) {
    fs::path physicalPath = directory;

    if (!physicalPath.is_absolute() && !root.empty()) {
        physicalPath = fs::path(root) / physicalPath;
    }

    physicalPath = fs::absolute(physicalPath).lexically_normal();

    if (!fs::exists(physicalPath) 
    ||  !fs::is_directory(physicalPath)
    ) {
        return false;
    }

    auto mount = std::make_unique<Mount>();

    mount->type = Mount::Type::Directory;
    mount->virtualPath = normalizePath(virtualPath);
    mount->directory = physicalPath.string();

    mounts.insert(
        mounts.begin(),
        std::move(mount)
    );

    return true;
}

bool FileSystem::mountPak(
    const std::string& virtualPath,
    const std::string& pakPath
) {
    fs::path physicalPath = pakPath;

    if (!physicalPath.is_absolute() && !root.empty()) {
        physicalPath = fs::path(root) / physicalPath;
    }

    physicalPath = fs::absolute(physicalPath).lexically_normal();

    auto pak = std::make_shared<PakArchive>();

    if (!pak->load(physicalPath.string())) {
        return false;
    }

    // pak->printFiles();

    auto mount = std::make_unique<Mount>();

    mount->type = Mount::Type::Pak;
    mount->virtualPath = normalizePath(virtualPath);
    mount->pak = pak;

    mounts.insert(
        mounts.begin(),
        std::move(mount)
    );

    return true;
}

std::string FileSystem::normalizePath(const std::string& path) const {
    std::string result = path;

    std::replace(
        result.begin(),
        result.end(),
        '\\',
        '/'
    );

    if (result.rfind("res://", 0) == 0) {
        result = result.substr(6);
    }

    const size_t firstNonSlash = result.find_first_not_of('/');
    if (firstNonSlash == std::string::npos) {
        result.clear();
    } else if (firstNonSlash > 0) {
        result.erase(0, firstNonSlash);
    }

    fs::path normalized = fs::path(result).lexically_normal();
    result = normalized.generic_string();

    if (result == ".") {
        result.clear();
    }

    while (result.rfind("../", 0) == 0) {
        result.erase(0, 3);
    }

    if (result == "..") {
        result.clear();
    }

    return result;
}

bool FileSystem::isWithinRoot(const std::string& path) const {
    fs::path rootPath = fs::absolute(root).lexically_normal();

    fs::path targetPath = fs::absolute(path).lexically_normal();

    auto rootIt = rootPath.begin();
    auto targetIt = targetPath.begin();

    for (; rootIt != rootPath.end(); ++rootIt, ++targetIt) {
        if (targetIt == targetPath.end()) {
            return false;
        }

        if (*rootIt != *targetIt) {
            return false;
        }
    }

    return true;
}

bool FileSystem::exists(const std::string& path) const {
    const std::string normalized = normalizePath(path);

    for (const auto& mount : mounts) {
        const auto relative = relativePathForMount(mount->virtualPath, normalized);
        if (!relative.has_value()) {
            continue;
        }

        const std::string& relativePath = *relative;

        if (mount->type == Mount::Type::Pak) {
            if (mount->pak->contains(relativePath)) {
                return true;
            }
            continue;
        }

        fs::path physical = fs::path(mount->directory) / relativePath;
        physical = fs::absolute(physical).lexically_normal();

        if (isWithinRoot(physical.string()) && fs::is_regular_file(physical)) {
            return true;
        }
    }

    return false;
}

std::unique_ptr<File> FileSystem::open(const std::string& path) const {
    const std::string normalized = normalizePath(path);

    for (const auto& mount : mounts) {
        const auto relative = relativePathForMount(mount->virtualPath, normalized);
        if (!relative.has_value()) {
            continue;
        }

        const std::string& relativePath = *relative;

        if (mount->type == Mount::Type::Pak) {
            auto file = mount->pak->open(relativePath);
            if (file) {
                return file;
            }
            continue;
        }

        fs::path physical = fs::path(mount->directory) / relativePath;
        physical = fs::absolute(physical).lexically_normal();

        if (!isWithinRoot(physical.string())
        ||  !fs::exists(physical)
        ||  !fs::is_regular_file(physical)
        ) {
            continue;
        }

        auto file = std::make_unique<DirectoryFile>(physical);
        if (file->isOpen()) {
            return file;
        }
    }

    return nullptr;
}

std::vector<uint8_t> FileSystem::readAll(const std::string& path) const {
    auto file = open(path);
    if (!file) {
        return {};
    }

    const int64_t fileSize = file->size();
    if (fileSize <= 0) {
        return {};
    }

    if (static_cast<uint64_t>(fileSize) > std::numeric_limits<size_t>::max()) {
        return {};
    }

    std::vector<uint8_t> data(static_cast<size_t>(fileSize));
    if (!file->readAll(data)) {
        return {};
    }

    return data;
}

std::vector<std::string> FileSystem::listFiles(const std::string& path) const {
    const std::string normalized = normalizePath(path);
    std::vector<std::string> files;

    for (const auto& mount : mounts) {
        const auto relative = relativePathForMount(mount->virtualPath, normalized);
        if (!relative.has_value()) {
            continue;
        }

        const std::string& relativePath = *relative;

        if (mount->type == Mount::Type::Pak) {
            auto mountedFiles = mount->pak->listFiles(relativePath);
            for (auto& file : mountedFiles) {
                if (!mount->virtualPath.empty()) {
                    file = mount->virtualPath + "/" + file;
                }
                files.push_back(std::move(file));
            }
            continue;
        }

        const fs::path physical = fs::path(mount->directory) / relativePath;
        std::error_code error;
        if (!isWithinRoot(fs::absolute(physical).lexically_normal().string())
        ||  !fs::is_directory(physical, error)) {
            continue;
        }

        for (const auto& entry : fs::directory_iterator(physical, error)) {
            if (error) {
                return {};
            }

            if (!entry.is_regular_file(error)) {
                continue;
            }

            std::string file = entry.path().filename().generic_string();
            if (!mount->virtualPath.empty()) {
                file = mount->virtualPath + "/" + relativePath + "/" + file;
            } else if (!relativePath.empty()) {
                file = relativePath + "/" + file;
            }
            files.push_back(std::move(file));
        }
    }

    std::sort(files.begin(), files.end());
    files.erase(
        std::unique(files.begin(), files.end()),
        files.end()
    );
    return files;
}

std::string FileSystem::resolve(const std::string& path) const {
    const std::string normalized = normalizePath(path);

    for (const auto& mount : mounts) {
        const auto relative = relativePathForMount(mount->virtualPath, normalized);
        if (!relative.has_value()) {
            continue;
        }

        const std::string& relativePath = *relative;

        if (mount->type != Mount::Type::Directory) {
            continue;
        }

        fs::path physical = fs::path(mount->directory) / relativePath;
        physical = fs::absolute(physical).lexically_normal();

        if (isWithinRoot(physical.string()) && fs::exists(physical)) {
            return physical.string();
        }
    }

    return {};
}

bool File::readAll(std::vector<uint8_t>& output) {
    const int64_t fileSize = size();
    if (fileSize < 0) {
        return false;
    }

    if (static_cast<uint64_t>(fileSize) > std::numeric_limits<size_t>::max()) {
        return false;
    }

    output.resize(static_cast<size_t>(fileSize));
    if (fileSize == 0) {
        return true;
    }

    if (read(output.data(), output.size()) != output.size()) {
        output.clear();
        return false;
    }

    return true;
}

} // namespace Tenshi