#include "io_mgr.hpp"

namespace ccad {

IoMgr::PCB_FILE_T IoMgr::guessPluginTypeFromExt(const std::string& path) const {
    if (path.length() >= 10 && path.substr(path.length() - 10) == ".kicad_pcb") {
        return PCB_FILE_T::KICAD_SEXP;
    }
    if (path.length() >= 4 && path.substr(path.length() - 4) == ".brd") {
        // Very rudimentary guess for legacy/eagle etc.
        return PCB_FILE_T::LEGACY; 
    }
    return PCB_FILE_T::UNKNOWN;
}

} // namespace ccad
