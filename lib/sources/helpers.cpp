#include <limits>
#include <array>
#include <bit>

#include "darctool.hpp"
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

std::string darc::magic_to_string() {
	std::string magicstr;
	
	for (auto c : std::bit_cast<std::array<char,
		sizeof(uint32_t)>>(header.magic))
	{
		magicstr += c;
	}
	return magicstr;
}

// for darctool

std::map<darctool::return_code, std::string> darctool::return_codes {
	{OK, "No errors"},
	{FAIL_OPEN_OUTPUT, "Failed to open the output archive file"},
	{FAIL_OPEN_INPUT, "Failed to open the file for reading"},
	{NO_STARTENTRY, "The first table actual entry wasn't initialized. The input root directory is likely empty"},
	{NO_MEM, "Failed to allocate memory for the table entry"},
	{INVALID_FS, "Invalid FS object type"},
	{FILESYSTEM_ERROR, "std::filesystem produced an error"},
};
