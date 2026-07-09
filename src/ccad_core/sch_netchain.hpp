#ifndef CCAD_CORE_SCH_NETCHAIN_HPP
#define CCAD_CORE_SCH_NETCHAIN_HPP

#include "sch_connection.hpp"
#include <vector>

namespace ccad {

// Represents a connected graph component (a 'chain' of connections) forming a net.
class SchNetChain {
public:
    SchNetChain() = default;

    // Add a connection to the chain
    void addConnection(const SchConnection& conn);

    const std::vector<SchConnection>& getConnections() const { return connections; }

private:
    std::vector<SchConnection> connections;
};

} // namespace ccad

#endif // CCAD_CORE_SCH_NETCHAIN_HPP
