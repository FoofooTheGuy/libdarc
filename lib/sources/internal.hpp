#pragma once

#include <cstdint>
#include <array>

// we're in c++20 so here's a byteswap implementation
// https://en.cppreference.com/w/cpp/numeric/byteswap.html
template<std::integral T>
constexpr T byteswap(T value) noexcept
{
	static_assert(std::has_unique_object_representations_v<T>, 
				  "T may not have padding bits");
	auto value_representation = std::bit_cast<std::array<std::byte, sizeof(T)>>(value);
	auto reversed = value_representation;
	for (std::size_t i = 0; i < sizeof(T); i++) {
		reversed[i] = value_representation[sizeof(T) - i - 1];
	}
	for (std::size_t i = 0; i < sizeof(T); i++) {
		value_representation[i] = reversed[i];
	}
	return std::bit_cast<T>(value_representation);
}
