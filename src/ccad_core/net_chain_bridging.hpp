#ifndef CCAD_CORE_NET_CHAIN_BRIDGING_HPP
#define CCAD_CORE_NET_CHAIN_BRIDGING_HPP

#include <cstdint>
#include <string>
#include <vector>

namespace ccad {

class Board;
class Track;
class Project;

struct NetChainBridge {
    std::string component_id;
    std::string pad1_id;
    std::string pad2_id;
    int64_t bridge_length_nm = 0;
    bool is_valid = false;
};

struct NetChainBridgingReport {
    std::string kicad_source;
    std::string parity_scope;
    std::string net_id;
    std::vector<NetChainBridge> bridges;
    std::vector<std::string> diagnostics;
    std::vector<std::string> pending_kicad_features;
};

NetChainBridgingReport calculateNetChainBridges(const Project& project, const std::string& net_id);

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
