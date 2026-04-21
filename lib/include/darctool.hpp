#pragma once

#include <iostream>
#include <fstream>

class darctool {
	public:
		// write a darc file
		// directory: input directory to build darc from
		// darc_file: output darc file
		// root_directory: name for the root dirirectory of the darc, usually "." (UTF-16)
		// return:
		//  
		static int write_darc(const std::string directory, const std::string darc_file, const std::string root_directory = ".");
};
