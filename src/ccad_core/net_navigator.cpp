#include "net_navigator.hpp"

namespace ccad {

NetNavigator::NetNavigator(const ConnectionGraph& graph) : graph_(graph) {}

SchNetChain NetNavigator::traceNet(const std::string& net_name) const {
    SchNetChain chain;
    auto conns = graph_.getConnections(net_name);
    for (const auto& conn : conns) {
        chain.addConnection(conn);
    }
    return chain;
}

} // namespace ccad
