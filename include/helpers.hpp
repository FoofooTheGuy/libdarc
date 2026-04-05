#pragma once

#include <iostream>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <string>
#include <vector>

std::string readUTF16str(std::ifstream* input, uint32_t offset);

std::string UTF8toUTF16(const std::string input);
std::string UTF16toUTF8(const std::string& input);

size_t nnc_utf16_to_utf8(uint8_t *out, size_t outlen, const uint16_t *in, size_t inlen);
size_t nnc_utf8_to_utf16(uint16_t *out, size_t outlen, const uint8_t *in, size_t inlen);
