#ifndef CCAD_CORE_HIERARCHICAL_SHEET_PARSER_HPP
#define CCAD_CORE_HIERARCHICAL_SHEET_PARSER_HPP

#include <string>
#include <vector>

namespace ccad {

struct Board;

// Represents a logical grouping of components derived from a specific schematic sheet instance
struct HierarchicalRoom {
    std::string roomName;
    std::string sheetPathUuid;
    std::vector<std::string> componentIds;
};

// Parses hierarchical path metadata from footprints to group them into layout rooms
class HierarchicalSheetParser {
public:
    explicit HierarchicalSheetParser(Board* board);
    ~HierarchicalSheetParser() = default;

    // Scans all footprints on the board and extracts their sheet path UUIDs
    // Groups components sharing the same sheet path into distinct Room structures
    bool parseRooms();

    // Retrieves the currently parsed logical rooms
    std::vector<HierarchicalRoom> getRooms() const;

    // Retrieves a specific room by its UUID
    bool getRoomByUuid(const std::string& uuid, HierarchicalRoom& outRoom) const;

private:
    Board* board_ = nullptr;
    std::vector<HierarchicalRoom> rooms_;
};

} // namespace ccad

#endif // CCAD_CORE_HIERARCHICAL_SHEET_PARSER_HPP
