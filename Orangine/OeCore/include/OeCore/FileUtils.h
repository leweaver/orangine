#pragma once

#include <fstream>
#include "EngineUtils.h"

// from: http://insanecoding.blogspot.com/2011/11/how-to-read-in-file-in-c.html
inline std::string get_file_contents(const wchar_t* filename)
{
	std::ifstream in(filename, std::ios::in | std::ios::binary);
	if (in)
	{
		std::string contents;
		in.seekg(0, std::ios::end);
		assert(in.tellg() >= 0);
		contents.resize(static_cast<const unsigned int>(in.tellg()));
		in.seekg(0, std::ios::beg);
		in.read(&contents[0], contents.size());
		in.close();
		return(contents);
	}
	throw std::runtime_error("Failed to read file: " + oe::errno_to_str());
}