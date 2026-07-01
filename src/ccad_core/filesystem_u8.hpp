#pragma once
#include <filesystem>
#include <string>

#ifdef _WIN32
#include <windows.h>
#endif

namespace ccad {

inline std::filesystem::path u8ToPath(const std::string& utf8_str) {
#ifdef _WIN32
  if (utf8_str.empty()) return std::filesystem::path();
  int size_needed = MultiByteToWideChar(CP_UTF8, 0, utf8_str.data(), (int)utf8_str.size(), NULL, 0);
  std::wstring wstrTo(size_needed, 0);
  MultiByteToWideChar(CP_UTF8, 0, utf8_str.data(), (int)utf8_str.size(), &wstrTo[0], size_needed);
  return std::filesystem::path(wstrTo);
#else
  return std::filesystem::path(utf8_str);
#endif
}

inline std::string pathToU8(const std::filesystem::path& path) {
#ifdef _WIN32
  std::wstring wstr = path.wstring();
  if (wstr.empty()) return std::string();
  int size_needed = WideCharToMultiByte(CP_UTF8, 0, wstr.data(), (int)wstr.size(), NULL, 0, NULL, NULL);
  std::string strTo(size_needed, 0);
  WideCharToMultiByte(CP_UTF8, 0, wstr.data(), (int)wstr.size(), &strTo[0], size_needed, NULL, NULL);
  return strTo;
#else
  return path.string();
#endif
}

} // namespace ccad
