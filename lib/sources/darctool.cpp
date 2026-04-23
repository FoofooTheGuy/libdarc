#include <filesystem>
#include <cstring>
#include <vector>

#include "utf.hpp"
#include "darctool.hpp"

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

static uint32_t g_table_size = 0;
static uint32_t g_filenametable_offset = 0;
static uint32_t g_total_table_entries = 0;
static uint32_t g_filedata_offset = 0;
static uint32_t g_filenametable_curentryoffset = 0;

static std::string g_basearc_path;

static std::vector<darcbuild_table_entry*> g_table_ptr_vector;

static std::vector<uint16_t> g_root_dir_name;

darctool::return_code build_darc_table(darcbuild_table_entry *startentry) {
	uint32_t direntry_index = 0;
	
	darcbuild_table_entry *curentry = startentry;
	darcbuild_table_entry *newentry = NULL;
	
	// list contents
	std::error_code error;
	// https://stackoverflow.com/a/612176
	for(const auto& entry : std::filesystem::directory_iterator(std::filesystem::path((const char8_t*)&*std::string(g_basearc_path).c_str()), error)) {
		if(std::filesystem::exists(entry, error)) {
			std::string entrystr = entry.path().filename().string();
			std::string fullentrystr = entry.path().string();
			
			//printf("dir name: %s\n", entrystr.c_str());
			
			if(direntry_index) {
				newentry = new darcbuild_table_entry;
				if (newentry == NULL) {
					//printf("Failed to allocate memory for the table entry.\n");
					return darctool::return_code::NO_MEM;
				}
				g_table_ptr_vector.push_back(newentry);
				newentry = {};
				
				curentry->next = newentry;
				curentry = newentry;
				newentry = NULL;
			}
			
			// initialize curentry
			curentry->fs_path = fullentrystr;
			curentry->initialized = 1;
			curentry->arc_name = UTF::convert8to16(entrystr);
			
			if(std::filesystem::is_directory(entry, error)) { // dir
				//puts("directory");
				
				curentry->entry.filename_offset = 0x01000000;
				
				g_basearc_path = fullentrystr + "/";
				
				//puts("new entry"); // dude
				newentry = new darcbuild_table_entry;
				if (newentry == NULL) {
					//printf("Failed to allocate memory for the table entry.\n");
					return darctool::return_code::NO_MEM;
				}
				g_table_ptr_vector.push_back(newentry);
				newentry = {};
				
				curentry->child = newentry;
				newentry->parent = curentry;
				
				build_darc_table(newentry); // recurse
				
				if(!newentry->initialized) { // The directory is likely empty.
					delete newentry;
					curentry->child = NULL;
				}
				
				newentry = NULL;
				
				//puts("loop");
			}
			else if(std::filesystem::is_regular_file(entry, error)) { // file
				//printf("file\n");
				curentry->entry.size = entry.file_size();
			}
			else {
				//printf("Invalid FS object type.\n");
				return darctool::return_code::INVALID_FS;
			}
			if(error) {
				std::cout << error.message() << std::endl;
				return darctool::return_code::FILESYSTEM_ERROR;
			}

			direntry_index++;
		}
	}
	if(error) {
		std::cout << error.message() << std::endl;
		return darctool::return_code::FILESYSTEM_ERROR;
	}

	return darctool::return_code::OK;
}

darctool::return_code update_darc_table(darcbuild_table_entry *startentry) {
	darcbuild_table_entry *curentry = startentry;
	darcbuild_table_entry *newentry;

	while(curentry) {
		curentry->entryindex = g_total_table_entries;
		
		//puts("write entry filename offset"); // dude
		
		curentry->entry.filename_offset = curentry->entry.filename_offset + g_filenametable_curentryoffset;
		
		g_filenametable_curentryoffset += curentry->arc_name.size();
		
		g_total_table_entries++;
		
		if (curentry->child) {
			update_darc_table(curentry->child); // recurse
		}
		
		newentry = startentry->parent;
		while(newentry) {
			newentry->total_children++;
			newentry = newentry->parent;
		}
		
		curentry = curentry->next;
	}
	
	return darctool::return_code::OK;
}

darctool::return_code update_darc_table_final(darcbuild_table_entry *startentry) {
	darcbuild_table_entry *curentry = startentry;
	
	while(curentry) {
		if((curentry->entry.filename_offset & 0x01000000) == 0) { // Check whether this entry is a file.
			
			g_filedata_offset = (g_filedata_offset + 0x7f) & ~0x7f; // Align the offset for .bclim(GPU texture files) to 0x80-bytes. This is the same alignment required by the GPU for textures' addresses.
			
			curentry->entry.offset = g_filedata_offset; // Setup file data offset.
			g_filedata_offset += curentry->entry.size;
			
			if (curentry->entryindex != g_total_table_entries - 1) {
				g_filedata_offset = (g_filedata_offset + 0x1f) & ~0x1f;
			}
		}
		else {
			curentry->entry.offset = 0x01; // Setup the directory start/end values.
			curentry->entry.size = curentry->entryindex + 1 + curentry->total_children;
		}
		
		if(curentry->child) {
			update_darc_table_final(curentry->child);
		}
		
		curentry = curentry->next;
	}
	
	return darctool::return_code::OK;
}

darctool::return_code build_darc(darcbuild_table_entry *startentry) {
	darctool::return_code ret = darctool::return_code::OK;
	
	ret = build_darc_table(startentry);
	if (ret != darctool::return_code::OK) {
		return ret;
	}
	
	if (!startentry->initialized) {
		//printf("Error: the first table actual entry wasn't initialized, the input root directory is likely empty.\n");
		return darctool::return_code::NO_STARTENTRY;
	}
	
	g_filenametable_curentryoffset = 0x2 + g_root_dir_name.size(); // length of dir names
	g_total_table_entries = 2;
	
	ret = update_darc_table(startentry);
	if (ret != darctool::return_code::OK) {
		return ret;
	}
	
	g_filenametable_offset = g_total_table_entries * sizeof(darc_table_entry);
	g_table_size = g_filenametable_offset + g_filenametable_curentryoffset;
	g_filedata_offset = 0x1c + g_table_size;
	
	ret = update_darc_table_final(startentry);
	
	return ret;
}

darctool::return_code writeout_table_entries(darcbuild_table_entry *startentry, int type, std::ofstream *farchive) {
	darcbuild_table_entry *curentry = startentry;
	
	while(curentry) {
		if (type == 0) { // table entries
			//std::cout << "write the table entry" << std::endl;
			farchive->write(reinterpret_cast<char*>(&curentry->entry), sizeof(darc_table_entry));
		}
		else if (type == 1) { // strings
			//std::cout << "write the object-name entry" << std::endl;
			farchive->write(reinterpret_cast<char*>(curentry->arc_name.data()), curentry->arc_name.size());
		}
		else if (type == 2) { // file data
			if ((curentry->entry.filename_offset & 0x01000000) == 0) { // Check whether this is a file.
				
				//uint32_t size = curentry->entry.size;
				
				uint32_t padding = curentry->entry.offset;
				
				//printf("padding %X\n", padding);
				
				//std::cout << "write filedata padding" << std::endl;
				
				while(farchive->tellp() < padding) {
					uint8_t i = 0;
					farchive->write(reinterpret_cast<char*>(&i), 1);
				}
				
				std::vector<uint8_t> filebuf;
				
				//std::cout << "open file: " << curentry->fs_path << std::endl;
				std::ifstream f(curentry->fs_path, std::ios_base::in | std::ios_base::binary);
				if (!f.is_open()) {
					//std::cout << "Failed to open the following file for reading: " << curentry->fs_path << std::endl;
					return darctool::return_code::FAIL_OPEN_INPUT;
				}
				
				f.seekg(0, std::ios_base::beg);
				
				// copy file data (without filling your memory)
				*farchive << f.rdbuf();
			}
		}
		
		if (curentry->child) {
			//puts("recurse"); // dude
			
			writeout_table_entries(curentry->child, type, farchive);
		}
		
		curentry = curentry->next;
	}
	
	return darctool::return_code::OK;
}

void free_g_table_ptrs() {
	for(const auto &p : g_table_ptr_vector) {
		delete p;
	}
}

darctool::return_code darctool::write_darc(const std::string directory, const std::string darc_file, const std::string root_directory) {
	return_code ret = return_code::OK;

	g_root_dir_name = UTF::convert8to16(root_directory);
	
	darc_header header = {};
	//memset(&header, 0, sizeof header);
	
	header.magicnum = 0x63726164;
	header.bom[0] = 0xFF;
	header.bom[1] = 0xFE;
	header.headerlen = 0x1C;
	header.version = 0x1000000;
	header.table_offset = 0x1C;
	
	darcbuild_table_entry buildtable_firstentry = {};
	
	g_basearc_path = directory + "/";
	
	//printf("Building the archive filesystem...\n");
	
	ret = build_darc(&buildtable_firstentry);
	
	if (ret != return_code::OK) {
		free_g_table_ptrs();
		return ret;
	}
	
	uint32_t real_filedata_offset = 0;
	
	if(buildtable_firstentry.child == NULL) {
		real_filedata_offset = buildtable_firstentry.entry.offset;
	}
	else {
		real_filedata_offset = buildtable_firstentry.child->entry.offset;
	}
		
	header.filesize = g_filedata_offset;
	header.table_size = real_filedata_offset - 0x1C;
	header.filedata_offset = real_filedata_offset;
	
	//puts("Writing the actual archive to the file...");
	
	//puts("write the darc header");
	
	std::ofstream farchive(darc_file, std::ios_base::out | std::ios_base::binary);
	if (!farchive.is_open()) {
		//puts("Failed to open the output archive file.");
		free_g_table_ptrs();
		return return_code::FAIL_OPEN_OUTPUT;
	}
	
	farchive.write(reinterpret_cast<char*>(&header), sizeof header);
	
	darc_table_entry tmpentries[2] = {};
	
	// Setup the first two entries in the darc: <null> and ".". Offset values is 0, so no need to write 0 again.
	tmpentries[0].filename_offset = 0x01000000 + 0x0;
	tmpentries[1].filename_offset = 0x01000000 + 0x2;
	tmpentries[0].size = g_total_table_entries;
	tmpentries[1].size = g_total_table_entries;
	
	//std::cout << "write the initial table entries" << std::endl;
	farchive.write(reinterpret_cast<char*>(&tmpentries), sizeof tmpentries);
	
	ret = writeout_table_entries(&buildtable_firstentry, 0, &farchive);
	if (ret != return_code::OK) {
		free_g_table_ptrs();
		return ret;
	}
	
	//puts("write the initial object-names");
	{
		uint16_t zero = 0;
		farchive.write(reinterpret_cast<char*>(&zero), sizeof zero); // NULL entry (first one)
		
		farchive.write(reinterpret_cast<char*>(g_root_dir_name.data()), g_root_dir_name.size());
	}
	
	//puts("write archive names");
	
	ret = writeout_table_entries(&buildtable_firstentry, 1, &farchive);
	if (ret != return_code::OK) {
		free_g_table_ptrs();
		return ret;
	}
	
	//puts("write the initial filedata padding");
	
	//printf("filedata_padding %X\n", filedata_padding);
	uint32_t filedata_padding = 0x1c + g_table_size;
	while(farchive.tellp() < filedata_padding) { // what does this actually do?
		uint8_t zero = 0;
		farchive.write(reinterpret_cast<char*>(&zero), sizeof zero);
	}
	
	//puts("write files data");
	
	ret = writeout_table_entries(&buildtable_firstentry, 2, &farchive);
	if (ret != return_code::OK) {
		free_g_table_ptrs();
		return ret;
	}
	
	free_g_table_ptrs();
	return ret;
}

std::string darctool::return_str(return_code ret) {
	return return_codes[ret];
}
