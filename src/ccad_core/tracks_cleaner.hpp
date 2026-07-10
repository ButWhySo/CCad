#ifndef CCAD_CORE_TRACKS_CLEANER_HPP
#define CCAD_CORE_TRACKS_CLEANER_HPP

namespace ccad {

class Board;

// Utility class to perform cleanup heuristics on board tracks
class TracksCleaner {
public:
    explicit TracksCleaner(Board* board);
    ~TracksCleaner() = default;

    // Removes dangling track segments that do not connect to pads or other tracks on both ends
    int cleanupDanglingTracks();

    // Merges collinear contiguous track segments into single longer segments
    int mergeCollinearTracks();

    // Removes tracks that are fully covered by or redundant to other tracks on the same net
    int cleanupRedundantTracks();

    // Runs all cleanup passes
    void cleanupBoard();

private:
    Board* board_ = nullptr;
};

} // namespace ccad

#endif // CCAD_CORE_TRACKS_CLEANER_HPP
