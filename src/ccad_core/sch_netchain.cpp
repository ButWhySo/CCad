#include "sch_netchain.hpp"

namespace ccad {

void SchNetChain::addConnection(const SchConnection& conn) {
    connections.push_back(conn);
}

} // namespace ccad
