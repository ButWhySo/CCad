#ifndef CCAD_CORE_PNS_BOARD_ADAPTER_HPP
#define CCAD_CORE_PNS_BOARD_ADAPTER_HPP

#include "pns_node.hpp"
#include <memory>
#include <string>
#include <cstdint>

namespace ccad {

struct Board;

// Builds a PNS obstacle view from board copper items for one active net/layer.
class PnsBoardObstacleIndex {
public:
    void rebuild(const Board& board, const std::string& active_net, const std::string& layer_id);
    bool blockedSegment(std::int64_t x1_nm, std::int64_t y1_nm,
                        std::int64_t x2_nm, std::int64_t y2_nm,
                        std::int64_t clearance_nm) const;
    std::size_t size() const { return node_.itemCount(); }

private:
    PnsNode node_;
    std::vector<std::shared_ptr<PnsItem>> items_;
};

} // namespace ccad

#endif
