#pragma once
#include <filesystem>
#include <string>
#include <string_view>

namespace drumator {
// Explicit byte-preserving UTF-8 conversion for C++20/23 char8_t.
// std::filesystem::u8path is deprecated in C++20 and /WX rejects it.
inline std::filesystem::path pathFromUtf8(std::string_view bytes) {
 std::u8string unicode;
 unicode.reserve(bytes.size());
 for(unsigned char c:bytes)unicode.push_back(static_cast<char8_t>(c));
 return std::filesystem::path(unicode);
}
inline std::string pathToUtf8(const std::filesystem::path& path) {
 const auto unicode=path.u8string();
 std::string bytes;
 bytes.reserve(unicode.size());
 for(char8_t c:unicode)bytes.push_back(static_cast<char>(c));
 return bytes;
}
}
