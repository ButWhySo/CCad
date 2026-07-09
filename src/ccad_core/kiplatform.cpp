#include "kiplatform.hpp"

namespace ccad {

std::string KiPlatform::getAppDataDir() {
    return "";
}

std::string KiPlatform::getOsName() {
#ifdef _WIN32
    return "Windows";
#elif __APPLE__
    return "macOS";
#elif __linux__
    return "Linux";
#else
    return "Unknown";
#endif
}

} // namespace ccad
