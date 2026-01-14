#pragma once

#include <cstdint>
#include <cstring>
#include <fstream>
#include <vector>
#include <map>

class darc {
	public:
		enum return_code {
			OK,
			INVALID_MAGIC,
			INVALID_VERSION,
			FAIL_OPEN,
			NOT_OPEN,
			FAIL_READ,
			FAIL_WRITE,
			TOO_SHORT,
			TOO_LONG,
			NO_MEM,
			INTERNAL,
		};
		
		enum endian { // endianess of the file
			little,
			big,
			mixed,
		};
		
		// read header and fill the table
		// file: active ifstream (open it before calling this)
		// return:
		// 	OK if good,
		// 	NOT_OPEN if file is not open,
		// 	TOO_SHORT if the stream is shorter than the header
		// 	INVALID_MAGIC if header magic is not "darc",
		//  INVALID_VERSION if the header version is not 0x01000000
		//  TOO_LONG if the file length said in the header is longer than the actual file,
		//  TOO_SHORT if the file length said in the header is shorter than the actual file,
		//  TOO_LONG if the table offset is not equal to the header length
		return_code initialize(std::ifstream* file);
		
		// return: entry_count
		uint32_t table_entries();
		// get filename pointer from table
		// entry: index of the table entry to read from
		// return: offset of filename from table entry
		uint32_t entry_filename(uint32_t entry);
		// get file pointer from table
		// entry: index of the table entry to read from
		// return: offset of file from table entry or darc::npos if it was a directory
		uint32_t entry_file(uint32_t entry);
		// get file length from table
		// entry: index of the table entry to read from
		// return: file length
		uint32_t entry_filelength(uint32_t entry);
		// check if a table entry is a directory 
		// entry: index of the table entry to read from
		// return:
		//  true for if it is a directory
		//  false for if it is a file
		bool entry_is_directory(uint32_t entry);
		
		// get endianess from header
		// return:
		//  little if little
		//	big if big
		endian get_endianess();
		
		// print info about darc
		// return:
		// 	OK
		return_code print_info();
		
		static std::string return_str(return_code ret);
	private:
		/* internal use & helper functions */
		
		typedef struct darc_header {
			uint32_t magic;            // Magic "darc"
			uint8_t endianess[2];      // Endianess FF FE: little endian, FE FF: Big endian
			uint16_t header_length;    // Header length in bytes
			uint32_t version;          // File version
			uint32_t file_length;      // File length in bytes
			uint32_t table_offset;     // Table offset (Absolute)
			uint32_t table_length;     // Table length in bytes
			uint32_t file_data_offset; // File data offset (Absolute)
		} darc_header;

		typedef struct darc_table_entry {
			uint32_t file_name_offset; // File name offset (Relative to the end of the entries of the table)
			uint32_t offset;           // File offset (Absolute)
			uint32_t length;           // File length
		} darc_table_entry;
		
		static const uint32_t DARC_MAGIC; // "darc"
		static const uint32_t DARC_VERSION;
		
		static std::map<return_code, std::string> return_codes;
		
		endian file_endianess;
		uint32_t entry_count; // derived from the length of the first entry
		darc_header header;
		std::vector<darc_table_entry> entries {{0, 0, 0}};
		
		// read 16 bits from stream
		// file: active ifstream (open it before calling this)
		// return:
		// 	OK if good
		// 	INTERNAL if platform is mixed endianess
		return_code readU16(std::ifstream* file, uint16_t* value);
		// read 32 bits from stream
		// file: active ifstream (open it before calling this)
		// return:
		// 	OK if good
		// 	INTERNAL if platform is mixed endianess
		return_code readU32(std::ifstream* file, uint32_t* value);
		
		// return: magic as a string ("darc")
		std::string magic_to_string();
};
