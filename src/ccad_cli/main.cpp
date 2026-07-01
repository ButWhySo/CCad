#include "ccad_cli/app.hpp"

#ifdef _WIN32
#include <windows.h>
#include <shellapi.h>
#include <vector>
#include <string>

std::string wstring_to_utf8(const std::wstring& wstr) {
  if (wstr.empty()) return std::string();
  int size_needed = WideCharToMultiByte(CP_UTF8, 0, wstr.data(), (int)wstr.size(), NULL, 0, NULL, NULL);
  std::string strTo(size_needed, 0);
  WideCharToMultiByte(CP_UTF8, 0, wstr.data(), (int)wstr.size(), &strTo[0], size_needed, NULL, NULL);
  return strTo;
}
#endif

int main(int argc, char** argv) {
#ifdef _WIN32
  int wargc = 0;
  LPWSTR* wargv = CommandLineToArgvW(GetCommandLineW(), &wargc);
  if (wargv) {
    std::vector<std::string> utf8_args;
    std::vector<char*> utf8_argv;
    utf8_args.reserve(wargc);
    utf8_argv.reserve(wargc);
    for (int i = 0; i < wargc; ++i) {
      utf8_args.push_back(wstring_to_utf8(wargv[i]));
    }
    for (int i = 0; i < wargc; ++i) {
      utf8_argv.push_back(&utf8_args[i][0]);
    }
    int result = ccad_cli::run(wargc, utf8_argv.data());
    LocalFree(wargv);
    return result;
  }
#endif
  return ccad_cli::run(argc, argv);
}

