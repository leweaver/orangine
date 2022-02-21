#pragma once

#include "EngineUtils.h"

#include <fstream>
#include <OeCore/EngineUtils.h>

// from: http://insanecoding.blogspot.com/2011/11/how-to-read-in-file-in-c.html
namespace oe {
inline std::string get_file_contents(const char* fileName) {
  auto filename_w = utf8_decode(fileName);
  std::ifstream in(filename_w, std::ios::in | std::ios::binary);
  if (in) {
    std::string contents;
    in.seekg(0, std::ios::end);
    OE_CHECK(in.tellg() >= 0);
    contents.resize(static_cast<const unsigned int>(in.tellg()));
    in.seekg(0, std::ios::beg);
    in.read(&contents[0], static_cast<int64_t>(contents.size()));
    in.close();
    return (contents);
  }
  OE_THROW(std::runtime_error("Failed to read file: " + oe::errno_to_str()));
}
} // namespace oe