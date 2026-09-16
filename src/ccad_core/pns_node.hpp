#ifndef CCAD_CORE_PNS_NODE_HPP
#define CCAD_CORE_PNS_NODE_HPP

#include "pns_index.hpp"
#include <vector>
#include <memory>
#include <string>
#include <utility>

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

class PnsSegmentItem final : public PnsItem {
public:
    void setSegment(int x1, int y1, int x2, int y2, int radius = 0) {
        setPosition((x1 + x2) / 2, (y1 + y2) / 2, radius);
        x1_ = x1; y1_ = y1; x2_ = x2; y2_ = y2;
    }
    int x1() const { return x1_; }
    int y1() const { return y1_; }
    int x2() const { return x2_; }
    int y2() const { return y2_; }
private:
    int x1_ = 0, y1_ = 0, x2_ = 0, y2_ = 0;
};

class PnsArcItem final : public PnsItem {
public:
    void setArc(int x1, int y1, int xm, int ym, int x2, int y2, int radius = 0) {
        setPosition((x1 + x2) / 2, (y1 + y2) / 2, radius);
        x1_ = x1; y1_ = y1; xm_ = xm; ym_ = ym; x2_ = x2; y2_ = y2;
    }
    int x1() const { return x1_; } int y1() const { return y1_; }
    int xm() const { return xm_; } int ym() const { return ym_; }
    int x2() const { return x2_; } int y2() const { return y2_; }
private:
    int x1_ = 0, y1_ = 0, xm_ = 0, ym_ = 0, x2_ = 0, y2_ = 0;
};

class PnsPolygonItem final : public PnsItem {
public:
    void setPolygon(std::vector<std::pair<int, int>> points, int radius = 0) {
        points_ = std::move(points);
        if (!points_.empty()) setPosition(points_.front().first, points_.front().second, radius);
    }
    const std::vector<std::pair<int, int>>& points() const { return points_; }
private:
    std::vector<std::pair<int, int>> points_;
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
    std::size_t itemCount() const { return items_.size(); }

private:
    PnsIndex index_;
    std::vector<std::shared_ptr<PnsItem>> items_;
};

} // namespace ccad

#endif // CCAD_CORE_PNS_NODE_HPP
