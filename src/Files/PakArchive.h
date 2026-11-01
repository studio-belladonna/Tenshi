#pragma once

#include "Tenshi/Files/FileSystem.h"

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace Tenshi {

class PakArchive {
public:
    PakArchive();

    bool load(const std::string& path);
    bool contains(const std::string& path) const;
    std::unique_ptr<File> open(const std::string& path) const;
    std::vector<std::string> listFiles(const std::string& path) const;

    uint64_t fileCount() const;
    void printFiles() const;

private:
    enum class Compression : uint32_t {
        None = 0,
        LHA = 1,
        Encrypted = 0x100
    };

    struct Entry {
        uint64_t offset;
        uint64_t compressedSize;
        uint64_t uncompressedSize;

        uint32_t compression;

        uint32_t checksum;
    };

    std::string archivePath;

    std::unordered_map<std::string, Entry> entries;

    std::string normalizePath(const std::string& path) const;

    bool readEntryData(const Entry& entry, std::vector<uint8_t>& output) const;
};

} // namespace Tenshi