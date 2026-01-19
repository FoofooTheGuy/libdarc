// Example program for libdarc

#include <filesystem>
#include <iostream>
#include <darc.hpp>

#include "helpers.hpp"

bool createDirectory(std::string dir) {
	//std::cout << dir << std::endl;
	std::error_code error(0, std::system_category());
	
	if(std::filesystem::exists(dir, error)) {
		if(!std::filesystem::is_empty(dir, error)) {
			std::cout << "Directory '" << dir << "' already exists and is not empty" << std::endl;
			return false;
		}
	}
	if(error) {
		return false;
	}
	
	std::filesystem::create_directory(dir, error);
	if(error) {
		return false;
	}
	return true;
}

void writeFile(std::ofstream* out, std::ifstream* in, uint32_t address, uint32_t length) {
	// store to reset when we're done
	size_t place = in->tellg();
	
	in->seekg(address, std::ios_base::beg);
	
	// https://stackoverflow.com/a/4063994
	const size_t buffer_size = 4096;
	char buffer[buffer_size];
	while (length > sizeof buffer) {
		in->read(buffer, buffer_size);
		out->write(buffer, buffer_size);
		length -= buffer_size;
	}
	in->read(buffer, length);
	out->write(buffer, length);
	
	in->seekg(place, std::ios_base::beg);
    return;
}

int main(int argc, char* argv[]) {
	if (argc < 3) {
		std::cout << "usage:\n" <<
		"extract archive: " << argv[0] << " <input .arc file> <new output directory>" << std::endl;
		return 1;
	}
	std::string outdir(argv[2]);
	std::ifstream infile(argv[1],
						 std::ios_base::in | std::ios_base::binary);
	darc arc;
	
	darc::return_code ret = arc.initialize(&infile);
	std::cout << "arc.initialize returned: " << darc::return_str(ret) << " (" << ret << ')' << std::endl;
	//arc.print_info();
	
	darc::endian endianess = arc.get_endianess();
	if (endianess == darc::endian::mixed) {
		std::cout << "bad endianess value" << std::endl;
		return 2;
	}
	else if (endianess == darc::endian::big) {
		std::cout << "We don't actually support big endian (yet?)" << std::endl;
		return 3;
	}
	else if (endianess == darc::endian::little) {
		std::cout << "little endian, good" << std::endl;
	}
	
	puts("");
	
	if (!createDirectory(outdir)) {
		std::cout << "failed to create dir '" << outdir << '\'' << std::endl;
		return 4;
	}
	// get absolute tree
	{
		bool previousdir = false;
		int nest = 0;
		std::vector<std::string> tree;
		for (uint32_t i = 1; i < arc.table_entries(); i++) {
			//std::string path = "";
			std::string outpath = outdir + '/';
			std::string entry = UTF16toUTF8(readUTF16str(&infile, arc.entry_filename(i))); // TODO: convert input to LE if necessary (i don't care)
			
			/*
			std::cout << "entry " << entry << " (" << arc.entry_is_directory(i) << ')' << std::endl;
			std::cout << "previous " << previousdir << std::endl;
			*/
			
			if (arc.entry_is_directory(i) && previousdir) { // subdir
				nest++;
				
				/*
				std::cout << "sub dir " << entry << std::endl;
				std::cout << "nest " << nest << std::endl;
				
				std::cout << "tree " << tree.size() << " {" << std::endl;
				for (const auto &s : tree) {
					std::cout << s << std::endl;
				}
				std::cout << '}' << std::endl;
				*/
			}
			else if (arc.entry_is_directory(i) && !previousdir) { // new dir
				if (nest) {
					tree.pop_back();
					nest--;
					
					/*
					std::cout << "new dir " << entry << std::endl;
					std::cout << "nest " << nest << std::endl;
					
					std::cout << "tree " << tree.size() << " {" << std::endl;
					for (const auto &s : tree) {
						std::cout << s << std::endl;
					}
					std::cout << '}' << std::endl;
					*/
				}
				else { // root dir
					nest++;
				}
			}
			
			if (arc.entry_is_directory(i)) {
				tree.push_back(entry);
			}
			
			for (auto s : tree) {
				if (strcmp(s.c_str(), ".") != 0) { // special root dir
					outpath += s + '/';
				}
			}
			if (!arc.entry_is_directory(i)) {
				outpath += entry;
			}
			
			std::cout << outpath << std::endl;
			
			if (arc.entry_is_directory(i)) {
				previousdir = true;
				
				if (!createDirectory(outpath)) {
					std::cout << "failed to create dir '" << outpath << '\'' << std::endl;
					return 5;
				}
			}
			else if (!arc.entry_is_directory(i)) {
				previousdir = false;

				std::ofstream out(outpath, std::ios_base::out | std::ios_base::binary);
				
				writeFile(&out, &infile, arc.entry_file(i), arc.entry_filelength(i));
			}
		}
	}
	return 0;
}
