#include <filesystem>
#include <iostream>
#include <darc.hpp>

#include "helpers.hpp"

bool createDirectory(std::string dir) {
	std::cout << dir << std::endl;
	std::error_code error(0, std::system_category());
	
	if(std::filesystem::exists(std::filesystem::path((const char8_t*)&*dir.c_str()), error)) {
		if(!std::filesystem::is_empty(std::filesystem::path((const char8_t*)&*dir.c_str()), error)) {
			std::cout << "Directory '" << dir << "' exists and is not empty" << std::endl;
			return false;
		}
	}
	if(error) {
		return false;
	}
	
	std::filesystem::create_directory(std::filesystem::path((const char8_t*)&*dir.c_str()), error);
	if(error) {
		return false;
	}
	return true;
}

void writeFile(std::string path, std::ifstream* in, uint32_t address, uint32_t length) {
	/*std::cout << path << std::endl;
	std::cout << length << std::endl;
	
	// store to reset when we're done
	size_t place = in->tellg();
	
	in->seekg(address, std::ios_base::beg);
	std::ofstream out(std::filesystem::path((const char8_t*)&*path.c_str()), std::ios_base::out | std::ios_base::binary);
	
	// https://stackoverflow.com/a/4063994
	const size_t buffer_size = 4096;
	char buffer[buffer_size];
	while (length >= sizeof buffer) {
		in->read(buffer, buffer_size);
		out.write(buffer, buffer_size);
		length -= buffer_size;
	}
	in->read(buffer, length);
	out.write(buffer, length);
	
	in->seekg(place, std::ios_base::beg);
    std::cout << "yo" << std::endl;*/
    return;
}

void what() {
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
	
	createDirectory(outdir);
	
	// get absolute tree
	{
		bool previousdir = false;
		int nest = 0;
		std::vector<std::string> tree;
		for (uint32_t i = 1; i < arc.table_entries(); i++) {
			std::string path = "";
			std::string entry = UTF16toUTF8(readUTF16str(&infile, arc.entry_filename(i))); // TODO: convert input to LE if necessary
			
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
			
			for (const auto &s : tree) {
				if (strcmp(s.c_str(), ".") != 0) { // special root dir
					path += s + '/';
				}
			}
			if (!arc.entry_is_directory(i)) {
				path += entry;
			}
			
			std::cout << path << std::endl;
			
			std::string outpath(outdir);
			outpath += '/';
			outpath += path;
			
			if (arc.entry_is_directory(i)) {
				previousdir = true;
				
				bool created = createDirectory(outdir + '/' + path);
				if(!created) {
					std::cout << "failed to create dir '" << outdir + '/' + path << '\'' << std::endl;
					return 4;
				}
			}
			else {
				previousdir = false;
				
				std::cout << "okay" << std::endl;
				//(void)writeFile(outpath, &infile, arc.entry_file(i), arc.entry_filelength(i));
				std::cout << "so?" << std::endl;
			}
		}
	}
	return 0;
}
