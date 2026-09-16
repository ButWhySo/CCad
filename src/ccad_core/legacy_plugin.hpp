#ifndef CCAD_CORE_LEGACY_PLUGIN_HPP
#define CCAD_CORE_LEGACY_PLUGIN_HPP

#include "io_mgr.hpp"
#include <string>

namespace ccad {

struct Board;

// Plugin for parsing legacy KiCad board formats (.brd)
class LegacyPlugin {
public:
    LegacyPlugin() = default;
    ~LegacyPlugin() = default;

    bool load(const std::string& filepath, Board* board);
};

} // namespace ccad

#endif // CCAD_CORE_LEGACY_PLUGIN_HPP
