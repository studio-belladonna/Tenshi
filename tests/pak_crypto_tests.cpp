#include "Files/PakCrypto.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <vector>

int main() {
    const std::array<std::uint8_t, 16> plaintext = {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
        0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F
    };
    std::vector<std::uint8_t> ciphertext;
    Tenshi::detail::PakNonce nonce{};
    Tenshi::detail::PakTag tag{};

    if (!Tenshi::detail::pakEncryptionEnabled()
    ||  !Tenshi::detail::pakEncrypt(plaintext, ciphertext, nonce, tag)
    ||  ciphertext.size() != plaintext.size()
    ) {
        return 1;
    }

    std::vector<std::uint8_t> decrypted;
    if (!Tenshi::detail::pakDecrypt(ciphertext, nonce, tag, decrypted)
    ||  decrypted.size() != plaintext.size()
    ||  !std::equal(decrypted.begin(), decrypted.end(), plaintext.begin())
    ) {
        return 2;
    }

    tag[0] ^= 0x01;
    if (Tenshi::detail::pakDecrypt(ciphertext, nonce, tag, decrypted)
    ||  !decrypted.empty()
    ) {
        return 3;
    }

    std::vector<std::uint8_t> emptyCiphertext;
    Tenshi::detail::PakNonce emptyNonce{};
    Tenshi::detail::PakTag emptyTag{};
    if (!Tenshi::detail::pakEncrypt(
            std::span<const std::uint8_t>(),
            emptyCiphertext,
            emptyNonce,
            emptyTag
        )
    ||  !emptyCiphertext.empty()
    ||  !Tenshi::detail::pakDecrypt(
            emptyCiphertext,
            emptyNonce,
            emptyTag,
            decrypted
        )
    ||  !decrypted.empty()
    ) {
        return 4;
    }

    return 0;
}
