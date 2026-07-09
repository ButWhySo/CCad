#ifndef CCAD_CORE_PNS_NODE_HPP
#define CCAD_CORE_PNS_NODE_HPP

#include "pns_index.hpp"

namespace ccad {

// Graph node for Push and Shove topology.
class PnsNode {
public:
    PnsNode() = default;

private:
    PnsIndex index_;
};

} // namespace ccad

#endif // CCAD_CORE_PNS_NODE_HPP
