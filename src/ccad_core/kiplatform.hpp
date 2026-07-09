#ifndef CCAD_CORE_KIPLATFORM_HPP
#define CCAD_CORE_KIPLATFORM_HPP

#include <string>

namespace ccad {

// Cross-platform wrappers for OS-specific behavior.
class KiPlatform {
public:
    static std::string getAppDataDir();
    static std::string getOsName();
};

} // namespace ccad

#endif // CCAD_CORE_KIPLATFORM_HPP
