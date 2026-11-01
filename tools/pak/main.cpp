#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <limits>
#include <cctype>
#include <charconv>
#include <span>

#include <liblha/lha.h>
#include "Files/PakCrypto.h"

namespace fs = std::filesystem;

constexpr uint32_t PAK_MAGIC = 0x4B415054;
constexpr uint32_t PAK_VERSION = 2;
constexpr uint32_t PAK_ENCRYPTED_VERSION = 3;

enum class Compression : uint32_t {
    None = 0,
    LHA = 1,
    Encrypted = 0x100
};

#pragma pack(push, 1)

struct PakHeader {
    uint32_t magic;
    uint32_t version;

    uint64_t fileCount;
    uint64_t tableOffset;
};

struct PakEntryHeader {
    uint32_t pathLength;

    uint64_t offset;
    uint64_t compressedSize;
    uint64_t uncompressedSize;

    uint32_t compression;
    uint32_t checksum;
};

#pragma pack(pop)

struct FileData {
    std::string path;

    std::vector<uint8_t> data;
    std::vector<uint8_t> compressed;
    std::vector<uint8_t> encrypted;

    Compression compression;

    uint32_t checksum;

    uint64_t offset = 0;
    Tenshi::detail::PakNonce nonce{};
    Tenshi::detail::PakTag tag{};
};

uint32_t crc32(const uint8_t* data, size_t size) {
    uint32_t crc = 0xFFFFFFFFu;

    for (size_t i = 0; i < size; ++i) {
        crc ^= data[i];

        for (int bit = 0; bit < 8; ++bit) {
            uint32_t mask = -(crc & 1u);
            crc = (crc >> 1) ^(0xEDB88320u & mask);
        }
    }

    return ~crc;
}

bool readFile(const fs::path& path, std::vector<uint8_t>& output) {
    std::ifstream file( path, std::ios::binary | std::ios::ate);
    if (!file) return false;
    
    std::streamsize size = file.tellg();
    if (size < 0) return false;

    file.seekg(0, std::ios::beg);

    output.resize(static_cast<size_t>(size));

    if (!output.empty()) {
        file.read(
            reinterpret_cast<char*>(output.data()),
            size
        );

        if (!file) return false;
    }

    return true;
}

std::string makeVirtualPath(const fs::path& root, const fs::path& file) {
    fs::path relative = fs::relative(file, root);

    return relative.generic_string();
}

bool compressFile(FileData& file) {
    if (file.data.empty()) {
        file.compression = Compression::None;
    } else {
        lha::Options options;
        options.maxOutputSize = file.data.size();
        lha::Result result = lha::compress(
            lha::Codec::Lh5,
            std::span<const lha::Byte>(file.data.data(), file.data.size()),
            options
        );
        if (!result) {
            std::cerr << "Failed to compress " << file.path << ": "
                << result.message << '\n';
            return false;
        }

        if (result.data.size() >= file.data.size()) {
            file.compressed.clear();
            file.compression = Compression::None;
        } else {
            file.compressed = std::move(result.data);
            file.compression = Compression::LHA;
        }
    }

    if (Tenshi::detail::pakEncryptionEnabled()) {
        const std::vector<uint8_t>& input
            = file.compression == Compression::LHA ? file.compressed : file.data;
        if (!Tenshi::detail::pakEncrypt(
            std::span<const uint8_t>(input.data(), input.size()),
            file.encrypted,
            file.nonce,
            file.tag
        )) {
            std::cerr << "Failed to encrypt " << file.path << '\n';
            return false;
        }

        file.compression = static_cast<Compression>(
            static_cast<uint32_t>(file.compression)
            | static_cast<uint32_t>(Compression::Encrypted)
        );
    }

    return true;
}

uint64_t storedSize(const FileData& file) {
    if ((static_cast<uint32_t>(file.compression)
        & static_cast<uint32_t>(Compression::Encrypted)) != 0) {
        return static_cast<uint64_t>(
            sizeof(file.nonce) + sizeof(file.tag) + file.encrypted.size()
        );
    }

    return file.compression == Compression::LHA
        ? static_cast<uint64_t>(file.compressed.size())
        : static_cast<uint64_t>(file.data.size());
}

uint64_t entryTableSize(const FileData& file) {
    return static_cast<uint64_t>(sizeof(PakEntryHeader))
        + static_cast<uint64_t>(file.path.size());
}

fs::path chunkPath(const fs::path& firstChunk, uint64_t index) {
    std::string stem = firstChunk.stem().string();
    while (!stem.empty()
        && std::isdigit(static_cast<unsigned char>(stem.back()))
    ) {
        stem.pop_back();
    }

    if (stem.empty()) {
        stem = "resource";
    }

    return firstChunk.parent_path()
        / (stem + std::to_string(index) + firstChunk.extension().string());
}

void removePreviousChunks(const fs::path& firstChunk) {
    const fs::path directory = firstChunk.parent_path();
    const std::string firstExtension = firstChunk.extension().string();

    std::string stem = firstChunk.stem().string();
    while (!stem.empty()
        && std::isdigit(static_cast<unsigned char>(stem.back()))) {
        stem.pop_back();
    }

    if (stem.empty() || !fs::exists(directory)) {
        return;
    }

    for (const auto& entry : fs::directory_iterator(directory)) {
        if (!entry.is_regular_file()
        ||  entry.path().extension() != firstExtension
        ) {
            continue;
        }

        const std::string candidate = entry.path().stem().string();
        if (candidate.rfind(stem, 0) != 0) {
            continue;
        }

        const std::string suffix = candidate.substr(stem.size());
        if (suffix.empty()
        ||  !std::all_of(suffix.begin(), suffix.end(), [](unsigned char c) {
                return std::isdigit(c);
            })
        ) {
            continue;
        }

        std::error_code error;
        fs::remove(entry.path(), error);
        if (error) {
            std::cerr << "Failed removing previous chunk: " << entry.path() << '\n';
        }
    }
}

bool writeChunk(const fs::path& outputFile, std::vector<FileData*>& entries) {
    std::ofstream output(outputFile, std::ios::binary | std::ios::trunc);
    if (!output) {
        std::cerr << "Failed to create output: " << outputFile << '\n';
        return false;
    }

    PakHeader header{};
    header.magic = PAK_MAGIC;
    header.version = Tenshi::detail::pakEncryptionEnabled()
        ? PAK_ENCRYPTED_VERSION : PAK_VERSION;
    header.fileCount = entries.size();
    output.write(reinterpret_cast<const char*>(&header), sizeof(header));

    for (FileData* entry : entries) {
        entry->offset = static_cast<uint64_t>(output.tellp());
        const bool encrypted = (static_cast<uint32_t>(entry->compression)
            & static_cast<uint32_t>(Compression::Encrypted)) != 0;
        const uint32_t compression = static_cast<uint32_t>(entry->compression)
            & ~static_cast<uint32_t>(Compression::Encrypted);
        const std::vector<uint8_t>& data = encrypted
            ? entry->encrypted
            : compression == static_cast<uint32_t>(Compression::LHA)
                ? entry->compressed : entry->data;
        if (encrypted) {
            output.write(
                reinterpret_cast<const char*>(entry->nonce.data()),
                static_cast<std::streamsize>(entry->nonce.size())
            );
            output.write(
                reinterpret_cast<const char*>(entry->tag.data()),
                static_cast<std::streamsize>(entry->tag.size())
            );
        }
        if (!data.empty()) {
            output.write(reinterpret_cast<const char*>(data.data()),
                static_cast<std::streamsize>(data.size())
            );
        }
    }

    header.tableOffset = static_cast<uint64_t>(output.tellp());
    for (const FileData* entry : entries) {
        PakEntryHeader entryHeader{};
        entryHeader.pathLength = static_cast<uint32_t>(entry->path.size());
        entryHeader.offset = entry->offset;
        entryHeader.compressedSize = storedSize(*entry);
        entryHeader.uncompressedSize = entry->data.size();
        entryHeader.compression = static_cast<uint32_t>(entry->compression);
        entryHeader.checksum = entry->checksum;

        output.write(
            reinterpret_cast<const char*>(&entryHeader),
            sizeof(entryHeader)
        );
        output.write(
            entry->path.data(),
            static_cast<std::streamsize>(entry->path.size())
        );
    }

    output.seekp(0, std::ios::beg);
    output.write(reinterpret_cast<const char*>(&header), sizeof(header));
    output.close();

    if (!output) {
        std::cerr << "Failed finalizing archive: " << outputFile << '\n';
        return false;
    }

    return true;
}

int main(int argc, char** argv) {
    if (argc != 3 && argc != 4) {
        std::cout
            << "Usage:\n"
            << "  tenshi_packer <input directory> <first output pak> [maximum chunk size in bytes]\n";

        return 1;
    }

    std::string inputString = fs::absolute(argv[1]).lexically_normal().string();
    
    // fix filepath bug when building with emscripten on windows
#ifdef _WIN32
    if (inputString.size() >= 3
    &&  inputString[0] == '/'
    &&  std::isalpha(static_cast<unsigned char>(inputString[1]))
    &&  inputString[2] == ':') {
        inputString.erase(0, 1);
    }
#endif

    std::filesystem::path inputDirectory(inputString);
    fs::path outputFile = fs::absolute(argv[2]).lexically_normal();
    uint64_t maximumChunkSize = std::numeric_limits<uint64_t>::max();

    if (argc == 4) {
        const std::string value = argv[3];
        const auto [end, error] = std::from_chars(value.data(), value.data() + value.size(), maximumChunkSize);
        if (error != std::errc() 
        ||  end != value.data() + value.size()
        ||  maximumChunkSize == 0
        ) {
            std::cerr << "Maximum chunk size must be a positive integer\n";
            return 1;
        }
    }

    if (!fs::exists(inputDirectory)
    ||  !fs::is_directory(inputDirectory)
    ) {
        std::cerr
            << "Input directory does not exist: "
            << inputDirectory
            << '\n';

        return 1;
    }

    std::vector<fs::path> files;

    for (const auto& entry : fs::recursive_directory_iterator(inputDirectory)) {
        if (!entry.is_regular_file()) {
            continue;
        }

        files.push_back(entry.path());
    }

    std::sort(files.begin(), files.end());

    std::vector<FileData> entries;

    for (const auto& path : files) {
        FileData file;
        file.path = makeVirtualPath(inputDirectory, path);

        std::cout
            << "Packing "
            << file.path
            << '\n';

        if (!readFile(path, file.data)) {
            std::cerr
                << "Failed to read: "
                << path
                << '\n';

            return 1;
        }

        file.checksum = crc32(file.data.data(), file.data.size());

        if (!compressFile(file)) {
            std::cerr
                << "Failed to compress: "
                << file.path
                << '\n';

            return 1;
        }

        entries.push_back(std::move(file));
    }

    removePreviousChunks(outputFile);

    std::vector<FileData*> chunk;
    uint64_t chunkSize = sizeof(PakHeader);
    uint64_t chunkIndex = 0;
    uint64_t archiveSize = 0;

    for (auto& entry : entries) {
        const uint64_t entrySize = storedSize(entry) + entryTableSize(entry);
        if (!chunk.empty() && chunkSize + entrySize > maximumChunkSize) {
            const fs::path path = chunkPath(outputFile, chunkIndex++);
            if (!writeChunk(path, chunk)) return 1;
            archiveSize += fs::file_size(path);
            chunk.clear();
            chunkSize = sizeof(PakHeader);
        }

        chunk.push_back(&entry);
        chunkSize += entrySize;
    }

    // always make resource0.tpk, even for an empty resource directory
    const fs::path path = chunkPath(outputFile, chunkIndex++);
    if (!writeChunk(path, chunk)) return 1;
    archiveSize += fs::file_size(path);

    uint64_t originalSize = 0;
    for (const auto& entry : entries) {
        originalSize += entry.data.size();
    }

    std::cout
        << '\n'
        << "Packed "
        << entries.size()
        << " files into "
        << chunkIndex
        << " chunk(s)\n"
        << "Original size: "
        << originalSize
        << " bytes\n"
        << "Archive size: "
        << archiveSize
        << " bytes\n";

    if (originalSize > 0) {
        double percentage
        =   100.0
        *   static_cast<double>(archiveSize)
        /   static_cast<double>(originalSize);

        std::cout
            << "Compression: "
            << percentage
            << "% of original\n";
    }

    std::cout
        << "Output: "
        << outputFile.parent_path()
        << '\n';

    return 0;
}