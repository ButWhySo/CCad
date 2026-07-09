#ifndef CCAD_CORE_SCH_CONNECTION_HPP
#define CCAD_CORE_SCH_CONNECTION_HPP

#include "model.hpp"
#include <string>
#include <vector>

namespace ccad {

// Represents a logical connection node in the schematic (e.g. a wire end, pin, or junction).
struct SchConnection {
    Point position;
    std::string net_name;
    int net_code = -1;
    bool is_driven = false;
};

// Represents an abstract graph of connectivity between schematic objects.
class ConnectionGraph {
public:
    ConnectionGraph() = default;

    // Build the connection graph from the schematic items
    void build(const Schematic& schematic);

    // Get all connections on a given net
    std::vector<SchConnection> getConnections(const std::string& net_name) const;

private:
    std::vector<SchConnection> connections;
};

} // namespace ccad

#endif // CCAD_CORE_SCH_CONNECTION_HPP
