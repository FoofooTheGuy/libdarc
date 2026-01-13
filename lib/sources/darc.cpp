#include <limits>
#include <array>

#include "darc.hpp"



darc::return_code darc::initialize(std::ifstream* file) {
	return_code ret;
	if (!file->is_open()) {
		return return_code::NOT_OPEN;
	}
	// Get actual file size https://stackoverflow.com/a/37808094
	file->seekg(0, std::ios_base::beg);
	file->ignore(std::numeric_limits<std::streamsize>::max());
	size_t realsize = file->gcount();
	file->clear(); //  Since ignore will have set eof.
	file->seekg(0, std::ios_base::beg);
	
	if (realsize < sizeof(darc_header)) {
		return return_code::TOO_SHORT;
	}
	
	ret = readU32(file, &header.magic);
	if (ret != return_code::OK) {
		return ret;
	}
	if (header.magic != DARC_MAGIC) {
		return return_code::INVALID_MAGIC;
	}
	
	file->read(
		reinterpret_cast<char*>(&header.endianess[0]),
		sizeof(header.endianess)
	);
	if (header.endianess[0] == 0xFF && header.endianess[1] == 0xFE) {
		file_endianess = endian::little; 
	}
	else if (header.endianess[0] == 0xFE && header.endianess[1] == 0xFF)
	{
		file_endianess = endian::big; 
	}
	
	ret = readU16(file, &header.header_length);
	if (ret != return_code::OK) {
		return ret;
	}
	
	ret = readU32(file, &header.version);
	if (ret != return_code::OK) {
		return ret;
	}
	if (header.version != DARC_VERSION) {
		return return_code::INVALID_VERSION;
	}

	ret = readU32(file, &header.file_length);
	if (ret != return_code::OK) {
		return ret;
	}
	if (header.file_length > realsize) {
		return return_code::TOO_LONG;
	}
	else if (header.file_length < realsize) {
		return return_code::TOO_SHORT;
	}

	ret = readU32(file, &header.table_offset);
	if (ret != return_code::OK) {
		return ret;
	}
	if (header.table_offset != header.header_length) { // is it wrong to assume this?
		return return_code::TOO_LONG;
	}

	ret = readU32(file, &header.table_length);
	if (ret != return_code::OK) {
		return ret;
	}

	ret = readU32(file, &header.file_data_offset);
	if (ret != return_code::OK) {
		return ret;
	}

	file->seekg(header.table_offset);
	ret = readU32(file, &entries.at(0).file_name_offset); // NULL entry
	if (ret != return_code::OK) {
		return ret;
	}

	ret = readU32(file, &entries.at(0).offset);
	if (ret != return_code::OK) {
		return ret;
	}

	ret = readU32(file, &entries.at(0).length);
	if (ret != return_code::OK) {
		return ret;
	}
	entry_count = entries.at(0).length;
	
	for (uint32_t i = 1; i < entry_count; i++) {
		uint32_t FILE_NAME_OFFSET;
		uint32_t OFFSET;
		uint32_t LENGTH;
		ret = readU32(file, &FILE_NAME_OFFSET);
		if (ret != return_code::OK) {
			return ret;
		}

		ret = readU32(file, &OFFSET);
		if (ret != return_code::OK) {
			return ret;
		}

		ret = readU32(file, &LENGTH);
		if (ret != return_code::OK) {
			return ret;
		}
		entries.push_back({FILE_NAME_OFFSET, OFFSET, LENGTH});
	}
	
	file->seekg(0, std::ios_base::beg);
	return return_code::OK;
}

uint32_t darc::table_entries() {
	return entry_count;
}

uint32_t darc::entry_filename(uint32_t entry) {
	uint32_t offset = entries[entry].file_name_offset & 0x00FFFFFF; // remove flag
	offset += sizeof(darc_header);
	offset += sizeof(darc_table_entry) * entry_count;

	return offset;
}

uint32_t darc::entry_file(uint32_t entry) {
	return entries[entry].offset;
}

uint32_t darc::entry_filelength(uint32_t entry) {
	return entries[entry].length;
}

bool darc::entry_is_directory(uint32_t entry) {
	uint32_t is_directory = entries[entry].file_name_offset;
	is_directory &= 0x01000000; // isolate flag
	if (is_directory) {
		return true;
	}

	return false;
}

darc::endian darc::get_endianess() {
	if (header.endianess[0] == 0xFF) {
		return little;
	}
	else if (header.endianess[0] == 0xFE) {
		return big;
	}
	return mixed; // I don't know
}

darc::return_code darc::print_info() {
	printf(
		"header:\n"
		" magic: 0x%04X (\"%s\")\n"
		" byte order mask: %02X %02X\n"
		" header length: 0x%02X\n"
		" version: 0x%08X\n"
		" file length: 0x%04X (%d)\n"
		" table offset: 0x%04X\n"
		" table length: 0x%04X (%d)\n"
		" file data offset: 0x%04X\n"
		"table: (\%d entries)\n"
		,
		header.magic, magic_to_string().c_str(),
		header.endianess[0], header.endianess[1],
		header.header_length,
		header.version,
		header.file_length, header.file_length,
		header.table_offset,
		header.table_length, header.table_length,
		header.file_data_offset,
		entry_count
	);
	return return_code::OK;
}

std::string darc::return_str(return_code ret) {
	return return_codes[ret];
}
