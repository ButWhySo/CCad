#ifndef CCAD_CORE_ALTIUM_PLUGIN_HPP
#define CCAD_CORE_ALTIUM_PLUGIN_HPP

#include "io_mgr.hpp"
#include <string>

namespace ccad {

class Board;

// Plugin for parsing Altium board formats (.PcbDoc)
class AltiumPlugin {
public:
    AltiumPlugin() = default;
    ~AltiumPlugin() = default;

    bool load(const std::string& filepath, Board* board);
};

} // namespace ccad

#endif // CCAD_CORE_ALTIUM_PLUGIN_HPP
