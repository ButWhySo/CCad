#ifndef CCAD_CORE_KICAD_PLUGIN_HPP
#define CCAD_CORE_KICAD_PLUGIN_HPP

#include "io_mgr.hpp"
#include <string>

namespace ccad {

struct Board;

// Plugin for parsing and saving KiCad S-expression files (.kicad_pcb)
class KicadPlugin {
public:
    KicadPlugin() = default;
    ~KicadPlugin() = default;

    bool load(const std::string& filepath, Board* board);
    bool save(const std::string& filepath, const Board* board);
};

} // namespace ccad

#endif // CCAD_CORE_KICAD_PLUGIN_HPP
