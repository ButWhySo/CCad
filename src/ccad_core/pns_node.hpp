#ifndef CCAD_CORE_PNS_NODE_HPP
#define CCAD_CORE_PNS_NODE_HPP

#include "pns_index.hpp"
#include <vector>
#include <memory>

namespace ccad {

// Item in the Push and Shove topology
class PnsItem {
public:
    virtual ~PnsItem() = default;
};

// Graph node for Push and Shove topology.
class PnsNode {
public:
    PnsNode() = default;
    ~PnsNode() = default;

    void addItem(std::shared_ptr<PnsItem> item);
    void clear();

private:
    PnsIndex index_;
    std::vector<std::shared_ptr<PnsItem>> items_;
};

} // namespace ccad

#endif // CCAD_CORE_PNS_NODE_HPP
