#include "Files/PakCrypto.h"

#include <algorithm>
#if defined(TENSHI_PAK_ENCRYPTION_ENABLED)
#include <sodium.h>

namespace Tenshi::detail {

std::array<std::uint8_t, 8> pakKeyPart0();
std::array<std::uint8_t, 8> pakKeyPart1();
std::array<std::uint8_t, 8> pakKeyPart2();
std::array<std::uint8_t, 8> pakKeyPart3();

namespace {

void clearOutput(std::vector<std::uint8_t>& output) noexcept {
    if (!output.empty()) {
        sodium_memzero(output.data(), output.size());
        output.clear();
    }
}

bool sodiumReady() noexcept {
    static const bool ready = sodium_init() >= 0;
    return ready;
}

std::array<std::uint8_t, 32> reconstructKey() {
    const std::array<std::array<std::uint8_t, 8>, 4> parts = {
        pakKeyPart0(),
        pakKeyPart1(),
        pakKeyPart2(),
        pakKeyPart3()
    };
    std::array<std::uint8_t, 32> key{};

    for (std::size_t index = 0; index < key.size(); ++index) {
        const std::uint8_t mask = static_cast<std::uint8_t>((index * 73 + 167) & 0xFF);
        key[index] = parts[index / 8][index % 8] ^ mask;
    }

    return key;
}

} // namespace

bool pakEncryptionEnabled() noexcept {
    return true;
}

bool pakEncrypt(std::span<const std::uint8_t> input, std::vector<std::uint8_t>& output, PakNonce& nonce, PakTag& tag) {
    clearOutput(output);
    if (!sodiumReady()) {
        return false;
    }

    randombytes_buf(nonce.data(), nonce.size());
    auto key = reconstructKey();
    output.resize(input.size() == 0 ? 1 : input.size());
    const std::uint8_t emptyInput = 0;
    const std::uint8_t* message = input.empty() ? &emptyInput : input.data();
    unsigned long long tagSize = 0;
    const int result = crypto_aead_xchacha20poly1305_ietf_encrypt_detached(
            output.data(),
            tag.data(),
            &tagSize,
            message,
            static_cast<unsigned long long>(input.size()),
            nullptr,
            0,
            nullptr,
            nonce.data(),
            key.data()
    );
    
    sodium_memzero(key.data(), key.size());
    if (result != 0
    ||  tagSize != static_cast<unsigned long long>(tag.size())) {
        clearOutput(output);
        return false;
    }

    output.resize(input.size());
    return true;
}

bool pakDecrypt(std::span<const std::uint8_t> input, const PakNonce& nonce, const PakTag& tag, std::vector<std::uint8_t>& output) {
    clearOutput(output);
    if (!sodiumReady()) {
        return false;
    }

    auto key = reconstructKey();
    output.resize(input.size() == 0 ? 1 : input.size());
    const std::uint8_t emptyInput = 0;
    const std::uint8_t* ciphertext = input.empty() ? &emptyInput : input.data();
    const int result = crypto_aead_xchacha20poly1305_ietf_decrypt_detached(
            output.data(),
            nullptr,
            ciphertext,
            static_cast<unsigned long long>(input.size()),
            tag.data(),
            nullptr,
            0,
            nonce.data(),
            key.data()
    );
    sodium_memzero(key.data(), key.size());
    if (result != 0) {
        clearOutput(output);
        return false;
    }

    output.resize(input.size());
    return true;
}

} // namespace Tenshi::detail

#else

namespace Tenshi::detail {

bool pakEncryptionEnabled() noexcept {
    return false;
}

bool pakEncrypt(std::span<const std::uint8_t>, std::vector<std::uint8_t>&, PakNonce&, PakTag&) {
    return false;
}

bool pakDecrypt(std::span<const std::uint8_t>, const PakNonce&, const PakTag&, std::vector<std::uint8_t>&) {
    return false;
}

} // namespace Tenshi::detail

#endif