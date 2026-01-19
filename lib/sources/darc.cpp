#include <limits>
#include <array>

#include "darc.hpp"


darc::return_code darc::initialize(std::ifstream* file) {
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
	
	read<uint32_t>(file, &header.magic);
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
	
	read<uint16_t>(file, &header.header_length);
	
	read<uint32_t>(file, &header.version);
	if (header.version != DARC_VERSION) {
		return return_code::INVALID_VERSION;
	}

	read<uint32_t>(file, &header.file_length);
	if (header.file_length > realsize) {
		return return_code::TOO_LONG;
	}
	else if (header.file_length < realsize) {
		return return_code::TOO_SHORT;
	}

	read<uint32_t>(file, &header.table_offset);
	if (header.table_offset != header.header_length) { // is it wrong to assume this?
		return return_code::TOO_LONG;
	}

	read<uint32_t>(file, &header.table_length);
	read<uint32_t>(file, &header.file_data_offset);

	file->seekg(header.table_offset);
	
	read<uint32_t>(file, &entries.at(0).file_name_offset); // read NULL entry
	read<uint32_t>(file, &entries.at(0).offset);
	read<uint32_t>(file, &entries.at(0).length);
	
	entry_count = entries.at(0).length;
	
	for (uint32_t i = 1; i < entry_count; i++) { // read the rest of the entries
		uint32_t FILE_NAME_OFFSET;
		uint32_t OFFSET;
		uint32_t LENGTH;
		read<uint32_t>(file, &FILE_NAME_OFFSET);
		read<uint32_t>(file, &OFFSET);
		read<uint32_t>(file, &LENGTH);
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
