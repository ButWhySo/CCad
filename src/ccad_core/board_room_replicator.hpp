#ifndef CCAD_CORE_BOARD_ROOM_REPLICATOR_HPP
#define CCAD_CORE_BOARD_ROOM_REPLICATOR_HPP

#include <string>

namespace ccad {

struct Board;
struct HierarchicalRoom;

// Handles stamping logic for copying the internal layout of one room to another
class BoardRoomReplicator {
public:
    explicit BoardRoomReplicator(Board* board);
    ~BoardRoomReplicator() = default;

    // Applies the relative layout (positions and rotations) of components inside the sourceRoom
    // onto the corresponding components inside the targetRoom.
    // The targetRoom's components are shifted relative to the targetOrigin.
    bool replicateLayout(const HierarchicalRoom& sourceRoom, const HierarchicalRoom& targetRoom, double targetOriginX, double targetOriginY);

private:
    Board* board_ = nullptr;
};

} // namespace ccad

#endif // CCAD_CORE_BOARD_ROOM_REPLICATOR_HPP
