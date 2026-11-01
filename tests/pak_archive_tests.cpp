#include "Files/PakArchive.h"

#include <vector>

int main(int argc, char** argv) {
    if (argc != 2) {
        return 1;
    }

    Tenshi::PakArchive archive;
    if (!archive.load(argv[1])) {
        return 2;
    }

    const std::vector<std::string> files = archive.listFiles("");
    if (files.empty()) {
        return 0;
    }

    std::unique_ptr<Tenshi::File> file = archive.open(files.front());
    if (!file) {
        return 3;
    }

    std::vector<std::uint8_t> contents;
    if (!file->readAll(contents)) {
        return 4;
    }

    return 0;
}