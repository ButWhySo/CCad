#include "altium_plugin.hpp"
#include "board.hpp"

namespace ccad {

bool AltiumPlugin::load(const std::string& filepath, Board* board) {
    if (!board || filepath.empty()) return false;
    
    // Stub for Altium .PcbDoc format parsing
    // Altium support is intentionally deferred in CCad until explicitly required
    return false;
}

} // namespace ccad
