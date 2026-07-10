#include "legacy_plugin.hpp"
#include "model.hpp"

namespace ccad {

bool LegacyPlugin::load(const std::string& filepath, Board* board) {
    if (!board || filepath.empty()) return false;
    
    // Stub for legacy .brd format parsing
    // Legacy support is intentionally deferred in CCad until explicitly required
    return false;
}

} // namespace ccad
