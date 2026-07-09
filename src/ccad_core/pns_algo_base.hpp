#ifndef CCAD_CORE_PNS_ALGO_BASE_HPP
#define CCAD_CORE_PNS_ALGO_BASE_HPP

#include "pns_node.hpp"
#include <memory>

namespace ccad {

// Base class for Push and Shove (PNS) routing algorithms.
class PnsAlgoBase {
public:
    PnsAlgoBase() = default;
    virtual ~PnsAlgoBase() = default;

    void setNode(std::shared_ptr<PnsNode> node);
    std::shared_ptr<PnsNode> getNode() const;

protected:
    std::shared_ptr<PnsNode> node_;
};

} // namespace ccad

#endif // CCAD_CORE_PNS_ALGO_BASE_HPP
