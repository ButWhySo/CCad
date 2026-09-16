#ifndef CCAD_CORE_PNS_NODE_HPP
#define CCAD_CORE_PNS_NODE_HPP

#include "pns_index.hpp"
#include <vector>
#include <memory>
#include <string>

namespace ccad {

// Item in the Push and Shove topology
class PnsItem {
public:
    virtual ~PnsItem() = default;

    void setPosition(int x, int y, int radius = 0) {
        x_ = x;
        y_ = y;
        radius_ = radius < 0 ? 0 : radius;
    }
    int x() const { return x_; }
    int y() const { return y_; }
    int radius() const { return radius_; }
    void setIdentity(std::string net_id, std::string layer_id) {
        net_id_ = std::move(net_id);
        layer_id_ = std::move(layer_id);
    }
    const std::string& netId() const { return net_id_; }
    const std::string& layerId() const { return layer_id_; }

private:
    int x_ = 0;
    int y_ = 0;
    int radius_ = 0;
    std::string net_id_;
    std::string layer_id_;
};

// Graph node for Push and Shove topology.
class PnsNode {
public:
    PnsNode() = default;
    ~PnsNode() = default;

    void addItem(std::shared_ptr<PnsItem> item);
    bool removeItem(PnsItem* item);
    void clear();
    std::vector<PnsItem*> query(int x, int y, int radius) const;
    std::vector<PnsItem*> querySegment(int x1, int y1, int x2, int y2, int clearance) const;
    std::vector<PnsItem*> querySegment(int x1, int y1, int x2, int y2, int clearance,
                                       const std::string& net_id, const std::string& layer_id) const;
    bool hasObstacle(int x, int y, int clearance) const;

private:
    PnsIndex index_;
    std::vector<std::shared_ptr<PnsItem>> items_;
};

} // namespace ccad

#endif // CCAD_CORE_PNS_NODE_HPP
