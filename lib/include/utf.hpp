#pragma once

#include <iostream>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <string>
#include <vector>

class UTF {
	public:
		static std::string readUTF16str(std::ifstream* input, uint32_t offset);

		static std::vector<uint16_t> convert8to16(const std::string input);
		static std::string convert16to8(const std::string& input);

	private:
		static size_t nnc_utf16_to_utf8(uint8_t *out, size_t outlen, const uint16_t *in, size_t inlen);
		static size_t nnc_utf8_to_utf16(uint16_t *out, size_t outlen, const uint8_t *in, size_t inlen);
};
