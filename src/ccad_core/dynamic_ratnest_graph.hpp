#ifndef CCAD_CORE_DYNAMIC_RATNEST_GRAPH_HPP
#define CCAD_CORE_DYNAMIC_RATNEST_GRAPH_HPP

#include <string>
#include <vector>
#include <unordered_map>

namespace ccad {

struct Board;

// Represents an unrouted pad or track endpoint that requires connectivity
struct RatnestNode {
    std::string nodeId;
    std::string netCode;
    double x;
    double y;
};

// Represents a calculated flying lead between two unconnected nodes on the same net
struct RatnestLine {
    std::string nodeA;
    std::string nodeB;
    std::string netCode;
};

// Maintains the core data structure of unrouted net nodes and their physical coordinates
class DynamicRatnestGraph {
public:
    explicit DynamicRatnestGraph(Board* board);
    ~DynamicRatnestGraph() = default;

    // Rebuilds the internal graph of unconnected nodes by scanning the board's pads and track segments
    void buildGraph();

    // Retrieves all nodes belonging to a specific net code
    std::vector<RatnestNode> getNodesForNet(const std::string& netCode) const;

    // Registers calculated flying leads to be rendered by the visualizer
    void setActiveRatnests(const std::vector<RatnestLine>& ratnests);

    // Retrieves the currently active flying leads
    std::vector<RatnestLine> getActiveRatnests() const;

private:
    Board* board_ = nullptr;
    std::unordered_map<std::string, std::vector<RatnestNode>> netNodes_;
    std::vector<RatnestLine> activeRatnests_;
};

} // namespace ccad

#endif // CCAD_CORE_DYNAMIC_RATNEST_GRAPH_HPP
