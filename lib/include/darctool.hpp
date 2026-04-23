#pragma once

#include <iostream>
#include <cstdint>
#include <fstream>
#include <vector>
#include <map>

class darctool {
	public:
		enum return_code {
			OK,
			FAIL_OPEN_OUTPUT,
			FAIL_OPEN_INPUT,
			NO_STARTENTRY,
			NO_MEM,
			INVALID_FS,
			FILESYSTEM_ERROR,
		};
		
		// write a darc file
		// directory: input directory to build darc from
		// darc_file: output darc file
		// root_directory: name for the root dirirectory of the darc, usually "." (UTF-16)
		// return:
		//  
		static return_code write_darc(const std::string directory, const std::string darc_file, const std::string root_directory = ".");
		
		static std::string return_str(return_code ret);
	private:
		typedef struct {
			uint32_t magicnum = 0; // 0x63726164 "darc"
			uint8_t bom[2] = {0, 0};
			uint16_t headerlen = 0;
			uint32_t version = 0;
			uint32_t filesize = 0;
			uint32_t table_offset = 0;
			uint32_t table_size = 0;
			uint32_t filedata_offset = 0;
		} darc_header;

		typedef struct {
			uint32_t filename_offset = 0;
			uint32_t offset = 0;
			uint32_t size = 0;
		} darc_table_entry;

		typedef struct _darcbuild_table_entry {
			uint32_t initialized = 0;
			uint32_t entryindex = 0;
			uint32_t total_children = 0;
			struct _darcbuild_table_entry *next, *child, *parent;
			darc_table_entry entry;
			std::string fs_path;
			std::vector<uint16_t> arc_name;
		} darcbuild_table_entry;

		static uint32_t g_table_size;
		static uint32_t g_filenametable_offset;
		static uint32_t g_total_table_entries;
		static uint32_t g_filedata_offset;
		static uint32_t g_filenametable_curentryoffset;
		
		static std::string g_basearc_path;
		static std::vector<darcbuild_table_entry*> g_table_ptr_vector;
		static std::vector<uint16_t> g_root_dir_name;
		
		static void free_g_table_ptrs();
		
		static return_code build_darc_table(darcbuild_table_entry *startentry);
		static return_code update_darc_table(darcbuild_table_entry *startentry);
		static return_code update_darc_table_final(darcbuild_table_entry *startentry);
		static return_code build_darc(darcbuild_table_entry *startentry);
		static return_code writeout_table_entries(darcbuild_table_entry *startentry, int type, std::ofstream *farchive);
		
		static std::map<return_code, std::string> return_codes;
};
