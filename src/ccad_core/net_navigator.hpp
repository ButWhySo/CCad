#ifndef CCAD_CORE_NET_NAVIGATOR_HPP
#define CCAD_CORE_NET_NAVIGATOR_HPP

#include "sch_connection.hpp"
#include "sch_netchain.hpp"

namespace ccad {

// A utility class to navigate hierarchical connectivity.
class NetNavigator {
public:
    explicit NetNavigator(const ConnectionGraph& graph);

    // Traces out a full net chain starting from a specific connection
    SchNetChain traceNet(const std::string& net_name) const;

private:
    const ConnectionGraph& graph_;
};

} // namespace ccad

#endif // CCAD_CORE_NET_NAVIGATOR_HPP
