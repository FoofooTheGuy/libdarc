#include <limits>
#include <array>

#include "internal.hpp"
#include "darc.hpp"

/* helper functions are here to avoid clutter in darc.cpp */

const uint32_t darc::DARC_MAGIC = 0x63726164; // "darc"
const uint32_t darc::DARC_VERSION = 0x01000000;

std::map<darc::return_code, std::string> darc::return_codes {
	{OK, "No errors"},
	{INVALID_MAGIC, "Magic is not the expected value"},
	{INVALID_VERSION, "Version is not the expected value"},
	{FAIL_OPEN, "Failed to open file"},
	{NOT_OPEN, "The given stream is not open"},
	{FAIL_READ, "Failed to read file"},
	{FAIL_WRITE, "Failed to write to file"},
	{TOO_SHORT,"The given size is too short"},
	{TOO_LONG, "The given size is too short"},
	{NO_MEM, "This program is out of memory to allocate"},
	{INTERNAL, "Internal program error, this should never happen"},
};

darc::return_code darc::readU16(std::ifstream* file, uint16_t* value) {
	file->read(
		reinterpret_cast<char*>(value),
		sizeof(uint16_t)
	);
	
	if constexpr (std::endian::native == std::endian::big) {
		if (file_endianess == endian::little) {
			*value = byteswap<uint16_t>(*value);
		}
	}
	else if constexpr (std::endian::native == std::endian::little) {
		if (file_endianess == endian::big) {
			*value = byteswap<uint16_t>(*value);
		}
	}
	else { // apparently this is possible but I'm not going to support it
		return return_code::INTERNAL;
	}
	return return_code::OK;
}

darc::return_code darc::readU32(std::ifstream* file, uint32_t* value) {
	file->read(
		reinterpret_cast<char*>(value),
		sizeof(uint32_t)
	);
	
	if constexpr (std::endian::native == std::endian::big) {
		if (file_endianess == endian::little) {
			*value = byteswap<uint32_t>(*value);
		}
	}
	else if constexpr (std::endian::native == std::endian::little) {
		if (file_endianess == endian::big) {
			*value = byteswap<uint32_t>(*value);
		}
	}
	else { // apparently this is possible but I'm not going to support it
		return return_code::INTERNAL;
	}
	return return_code::OK;
}

std::string darc::magic_to_string() {
	std::string magicstr;
	uint32_t magic = header.magic;
	
	if constexpr (std::endian::native == std::endian::big) {
		magic = byteswap<uint32_t>(magic); // does this work?
	}
	
	for (auto c : std::bit_cast<std::array<char,
		sizeof(uint32_t)>>(magic))
	{
		magicstr += c;
	}
	return magicstr;
}
