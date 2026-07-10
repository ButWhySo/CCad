#ifndef CCAD_CORE_NET_CHAIN_BRIDGING_HPP
#define CCAD_CORE_NET_CHAIN_BRIDGING_HPP

#include <vector>

namespace ccad {

class Board;
class Track;

// Analyzes the physical connectivity of tracks and assigns them to contiguous logical nets
class NetChainBridging {
public:
    explicit NetChainBridging(Board* board);
    ~NetChainBridging() = default;

    // Evaluates track intersections and assigns unassigned tracks to touching nets
    void bridgeTrackNets();

    // Returns a set of contiguous track chains that share the same logical net
    std::vector<std::vector<Track*>> getLogicalChains() const;

private:
    Board* board_ = nullptr;
};

} // namespace ccad

#endif // CCAD_CORE_NET_CHAIN_BRIDGING_HPP
