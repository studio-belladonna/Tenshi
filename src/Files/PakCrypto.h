#pragma once

#include <array>
#include <cstdint>
#include <span>
#include <vector>

namespace Tenshi::detail {

using PakNonce = std::array<std::uint8_t, 24>;
using PakTag = std::array<std::uint8_t, 16>;

bool pakEncryptionEnabled() noexcept;

bool pakEncrypt(std::span<const std::uint8_t> input, std::vector<std::uint8_t>& output, PakNonce& nonce, PakTag& tag);
bool pakDecrypt(std::span<const std::uint8_t> input, const PakNonce& nonce,const PakTag& tag, std::vector<std::uint8_t>& output);

} // namespace Tenshi::detail
