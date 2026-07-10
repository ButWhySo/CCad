#include "hierarchical_sheet_parser.hpp"
#include "model.hpp"

namespace ccad {

HierarchicalSheetParser::HierarchicalSheetParser(Board* board)
    : board_(board) {
}

bool HierarchicalSheetParser::parseRooms() {
    rooms_.clear();
    if (!board_) return false;
    
    // Stub:
    // 1. Iterate over all footprints on the board
    // 2. Read the "sheetpath" UUID property from each footprint
    // 3. Cluster footprints sharing the same sheetpath UUID into HierarchicalRoom structs
    // 4. Store resulting groups in rooms_
    
    return true;
}

std::vector<HierarchicalRoom> HierarchicalSheetParser::getRooms() const {
    return rooms_;
}

bool HierarchicalSheetParser::getRoomByUuid(const std::string& uuid, HierarchicalRoom& outRoom) const {
    for (const auto& room : rooms_) {
        if (room.sheetPathUuid == uuid) {
            outRoom = room;
            return true;
        }
    }
    return false;
}

} // namespace ccad
