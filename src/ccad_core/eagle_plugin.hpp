#ifndef CCAD_CORE_EAGLE_PLUGIN_HPP
#define CCAD_CORE_EAGLE_PLUGIN_HPP

#include "io_mgr.hpp"
#include <string>

namespace ccad {

class Board;

// Plugin for parsing XML Eagle board formats (.brd)
class EaglePlugin {
public:
    EaglePlugin() = default;
    ~EaglePlugin() = default;

    bool load(const std::string& filepath, Board* board);
};

} // namespace ccad

#endif // CCAD_CORE_EAGLE_PLUGIN_HPP
