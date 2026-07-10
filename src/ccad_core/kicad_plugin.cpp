#include "kicad_plugin.hpp"
#include "board.hpp"
#include "pcb_parser.hpp"

namespace ccad {

bool KicadPlugin::load(const std::string& filepath, Board* board) {
    if (!board || filepath.empty()) return false;
    
    // Defer to the PcbParser implementation which holds the actual S-expression logic
    PcbParser parser;
    return parser.parse(filepath, board);
}

bool KicadPlugin::save(const std::string& filepath, const Board* board) {
    if (!board || filepath.empty()) return false;
    
    // Stub for S-expression saving
    return true;
}

} // namespace ccad
