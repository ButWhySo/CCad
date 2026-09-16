#include "pns_board_adapter.hpp"
#include "model.hpp"

#include <algorithm>
#include <climits>

namespace ccad {

bool viaCoversLayer(const Via& via, const Board& board, const std::string& layer_id) {
    if (via.via_type == "through" || via.start_layer_id.empty() || via.end_layer_id.empty()) return true;
    auto start = std::find_if(board.layers.begin(), board.layers.end(), [&](const Layer& l) {
        return l.id == via.start_layer_id;
    });
    auto end = std::find_if(board.layers.begin(), board.layers.end(), [&](const Layer& l) {
        return l.id == via.end_layer_id;
    });
    auto active = std::find_if(board.layers.begin(), board.layers.end(), [&](const Layer& l) {
        return l.id == layer_id;
    });
    if (start == board.layers.end() || end == board.layers.end() || active == board.layers.end())
        return layer_id == via.start_layer_id || layer_id == via.end_layer_id;
    const auto first = std::min(start - board.layers.begin(), end - board.layers.begin());
    const auto last = std::max(start - board.layers.begin(), end - board.layers.begin());
    const auto current = active - board.layers.begin();
    return current >= first && current <= last;
}

void PnsBoardObstacleIndex::rebuild(const Board& board, const std::string& active_net,
                                    const std::string& layer_id) {
    node_.clear();
    items_.clear();
    for (const Pad& pad : board.pads) {
        if (pad.net_id.empty() || pad.net_id == active_net ||
            std::find(pad.padstack.layer_set.begin(), pad.padstack.layer_set.end(), layer_id) == pad.padstack.layer_set.end()) continue;
        auto item = std::make_shared<PnsItem>();
        const auto props = pad.padstack.copper_props.find(layer_id);
        const auto& size = props == pad.padstack.copper_props.end() ? Size{} : props->second.shape.size;
        const auto radius = std::max(size.width.nanometers, size.height.nanometers) / 2;
        item->setPosition(static_cast<int>(pad.position.x.nanometers),
                          static_cast<int>(pad.position.y.nanometers),
                          static_cast<int>(std::max<std::int64_t>(0, radius)));
        item->setIdentity(pad.net_id, layer_id);
        node_.addItem(item);
        items_.push_back(std::move(item));
    }
    for (const Via& via : board.vias) {
        if (via.net_id.empty() || via.net_id == active_net || !viaCoversLayer(via, board, layer_id)) continue;
        auto item = std::make_shared<PnsItem>();
        item->setPosition(static_cast<int>(via.position.x.nanometers),
                          static_cast<int>(via.position.y.nanometers),
                          static_cast<int>(std::max<std::int64_t>(0, via.diameter.nanometers / 2)));
        item->setIdentity(via.net_id, layer_id);
        node_.addItem(item);
        items_.push_back(std::move(item));
    }
}

bool PnsBoardObstacleIndex::blockedSegment(std::int64_t x1_nm, std::int64_t y1_nm,
                                           std::int64_t x2_nm, std::int64_t y2_nm,
                                           std::int64_t clearance_nm) const {
    return !blockingItems(x1_nm, y1_nm, x2_nm, y2_nm, clearance_nm).empty();
}

std::vector<const PnsItem*> PnsBoardObstacleIndex::blockingItems(
    std::int64_t x1_nm, std::int64_t y1_nm, std::int64_t x2_nm, std::int64_t y2_nm,
    std::int64_t clearance_nm) const {
    std::vector<const PnsItem*> result;
    if (x1_nm < INT_MIN || x1_nm > INT_MAX || y1_nm < INT_MIN || y1_nm > INT_MAX ||
        x2_nm < INT_MIN || x2_nm > INT_MAX || y2_nm < INT_MIN || y2_nm > INT_MAX ||
        clearance_nm < 0 || clearance_nm > INT_MAX) return result;
    for (PnsItem* item : node_.querySegment(static_cast<int>(x1_nm), static_cast<int>(y1_nm),
                                            static_cast<int>(x2_nm), static_cast<int>(y2_nm),
                                            static_cast<int>(clearance_nm)))
        result.push_back(item);
    return result;
}

} // namespace ccad
