#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>

namespace Tenshi {

struct Entity {
	using Index = std::uint32_t;
	static constexpr Index InvalidIndex = static_cast<Index>(-1);

	Index index = InvalidIndex;
	std::uint32_t generation = 0;

	bool valid() const noexcept {
		return index != InvalidIndex;
	}

	friend bool operator==(Entity, Entity) = default;
};

} // namespace Tenshi

template<>
struct std::hash<Tenshi::Entity> {
	std::size_t operator()(Tenshi::Entity entity) const noexcept {
		const std::size_t index = std::hash<Tenshi::Entity::Index>{}(entity.index);
		const std::size_t generation = std::hash<std::uint32_t>{}(entity.generation);
		return index ^ (generation + 0x9e3779b9U + (index << 6) + (index >> 2));
	}
};