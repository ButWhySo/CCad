#ifndef CCAD_CORE_IO_MGR_HPP
#define CCAD_CORE_IO_MGR_HPP

#include <string>

namespace ccad {

// IO_MGR interface for abstracting different PCB file format plugins.
// Modeled after KiCad's IO_MGR.
class IoMgr {
public:
    enum class PCB_FILE_T {
        KICAD_SEXP = 0,
        LEGACY,
        EAGLE,
        ALTIUM,
        UNKNOWN
    };

    IoMgr() = default;
    virtual ~IoMgr() = default;

    virtual PCB_FILE_T guessPluginTypeFromExt(const std::string& path) const;
};

} // namespace ccad

#endif // CCAD_CORE_IO_MGR_HPP
