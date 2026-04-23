#pragma once

#include <iostream>
#include <fstream>
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
		static std::map<return_code, std::string> return_codes;
};
