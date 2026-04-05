#include "helpers.hpp"

size_t chrcount(const std::string& str) {
	size_t length = 0;
	for (char c : str) {
		if ((c & 0xC0) != 0x80) {
			++length;
		}
	}
	return length;
}

std::string readUTF16str(std::ifstream* input, uint32_t offset) {

	if (!input->is_open()) {
		std::cout << "ERROR: file not open" << std::endl;
		return "";
	}
	
	input->seekg(offset);
	
	std::string line = "";
	char byte;
	while(input->read(reinterpret_cast<char*>(&byte), sizeof byte)) {
		line += byte;		
		if (byte == 0x00 && input->peek() == 0x00) { // UTF16 NULL terminator
			input->get();
			line += byte;
			input->get();
			line += byte; // 00
			break;
		}
	}
	return line;
}

std::string UTF8toUTF16(const std::string input) {
	std::vector<uint8_t> utf8 = std::vector<uint8_t>(input.size() + 1);
	std::vector<uint16_t> utf16 = std::vector<uint16_t>(chrcount(input) * 2);
	memcpy(utf8.data(), input.c_str(), input.size());
	utf8[input.size()] = '\0';
	nnc_utf8_to_utf16(utf16.data(), chrcount(input) * 2 + 1, utf8.data(), input.size());

	std::string output(reinterpret_cast<char*>(utf16.data()), chrcount(input) * 2);

	return output;
}

std::string UTF16toUTF8(const std::string& input) {
	size_t outLen = 0;
	size_t utf16length = input.size() / 2; // divide by 2 because it's a u8 size going into a u16 array
	
	std::cout << input << std::endl;
	for(const auto &c : input) {
		printf("%02X\n", c);
	}
	
	std::vector<uint16_t> utf16 = std::vector<uint16_t>(utf16length);
	memcpy(utf16.data(), input.data(), input.size());
	size_t utf8length = nnc_utf16_to_utf8(NULL, 0, utf16.data(), utf16length) + 1;
	std::vector<uint8_t> utf8 = std::vector<uint8_t>(utf8length);
	memset(utf8.data(), 0, utf8length);
	
	outLen = nnc_utf16_to_utf8(utf8.data(), utf8length, utf16.data(), utf16length);
	std::string output(reinterpret_cast<char*>(utf8.data()), outLen);
	
	return output;
}

// junk stolen from nnc
static void write_utf8(uint8_t *out, size_t outlen, size_t *outptr, uint32_t cp)
{
	if(cp < 0x80)
	{
		size_t n = *outptr + 1;
		if(n < outlen)
			out[*outptr] = cp;
		*outptr = n;
	}
	else if(cp < 0x800)
	{
		size_t n = *outptr + 2;
		if(n < outlen)
		{
			out[*outptr + 0] = (cp >> 6)   | 0xC0;
			out[*outptr + 1] = (cp & 0x3F) | 0x80;
		}
		*outptr = n;
	}
	else if(cp < 0x10000)
	{
		size_t n = *outptr + 3;
		if(n < outlen)
		{
			out[*outptr + 0] = (cp >> 12)         | 0xE0;
			out[*outptr + 1] = ((cp >> 6) & 0x3F) | 0x80;
			out[*outptr + 2] = (cp & 0x3F)        | 0x80;
		}
		*outptr = n;
	}
	else if(cp < 0x110000)
	{
		size_t n = *outptr + 4;
		if(n < outlen)
		{
			out[*outptr + 0] = (cp >> 18)          | 0xF0;
			out[*outptr + 1] = ((cp >> 12) & 0x3F) | 0x80;
			out[*outptr + 2] = ((cp >> 6) & 0x3F)  | 0x80;
			out[*outptr + 3] = (cp & 0x3F)         | 0x80;
		}
		*outptr = n;
	}
	else { } /* invalid codepoint */
}

static void write_utf16(uint16_t *out, size_t outlen, size_t *outptr, uint32_t cp)
{
	/* contains invalid codepoints as well, we'll just ignore them */
	if(cp < 0x10000)
	{
		size_t n = *outptr + 1;
		if(n < outlen)
			out[*outptr] = cp;
		*outptr = n;
	}
	else if(cp < 0x11000)
	{
		cp &= ~0x10000;
		size_t n = *outptr + 2;
		if(n < outlen)
		{
			out[*outptr + 0] = (cp >> 10)   | 0xD800;
			out[*outptr + 1] = (cp & 0x3FF) | 0xDC00;
		}
		*outptr = n;
	}
	else { } /* invalid codepoint */
}

#define LE16(a) ((uint16_t) (a))

/* should there be a BE version of this? */
size_t nnc_utf16_to_utf8(uint8_t *out, size_t outlen, const uint16_t *in, size_t inlen)
{
	size_t outptr = 0;
	for(size_t i = 0; i < inlen; ++i)
	{
		uint16_t p1 = LE16(in[i]);
		if(p1 == '\0')
			break; /* finished */
		else if(p1 < 0xD800 || p1 > 0xE000)
			write_utf8(out, outlen, &outptr, p1);
		/* surrogate pair */
		else
		{
			uint16_t p2 = LE16(in[i + 1]);
			uint16_t w1 = p1 & 0x3FF;
			uint16_t w2 = p2 & 0x3FF;
			uint32_t cp = 0x10000 | (w1 << 10) | w2;
			write_utf8(out, outlen, &outptr, cp);
			++i; /* since we moved ahead 2 for the pair */
		}
	}
	return outptr;
}

size_t nnc_utf8_to_utf16(uint16_t *out, size_t outlen, const uint8_t *in, size_t inlen)
{
	size_t outptr = 0;
	for(size_t i = 0; i < inlen; ++i)
	{
		uint8_t p1 = in[i];
		if(p1 == '\0')
			break; /* finished */
		else if(p1 < 0x80)
		{
			write_utf16(out, outlen, &outptr, in[i]);
			continue;
		}
#define INCCHK(n) if(!((i + n) < inlen)) break
		INCCHK(1);
		if(p1 < 0xE0)
		{
			uint32_t cp = ((p1 & 0x1F) << 6)
			       | (in[i + 1] & 0x3F);
			write_utf16(out, outlen, &outptr, cp);
			i += 1;
			continue;
		}
		INCCHK(2);
		if(p1 < 0xF0)
		{
			uint32_t cp = ((p1 & 0xF) << 12)
			       | ((in[i + 1] & 0x3F) << 6)
			       | (in[i + 2] & 0x3F);
			write_utf16(out, outlen, &outptr, cp);
			i += 2;
			continue;
		}
		INCCHK(3);
		if(p1 < 0xF5)
		{
			uint32_t cp = ((p1 & 0x7) << 18)
			       | ((in[i + 1] & 0x3F) << 12)
			       | ((in[i + 2] & 0x3F) << 6)
			       | (in[i + 3] & 0x3F);
			write_utf16(out, outlen, &outptr, cp);
			i += 3;
			continue;
		}
#undef INCCHK
		/* invalid... */
	}
	return outptr;
}
