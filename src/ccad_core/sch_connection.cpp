#include "sch_connection.hpp"

namespace ccad {

void ConnectionGraph::build(const Schematic& schematic) {
    // Stub implementation
    // In the future, this will build a full graph of all wires, pins, junctions,
    // and labels, computing the hierarchical net names for each sub-sheet connection.
    connections.clear();
}

std::vector<SchConnection> ConnectionGraph::getConnections(const std::string& net_name) const {
    std::vector<SchConnection> result;
    for (const auto& conn : connections) {
        if (conn.net_name == net_name) {
            result.push_back(conn);
        }
    }
    return result;
}

} // namespace ccad
