#include "pns_algo_base.hpp"

namespace ccad {

void PnsAlgoBase::setNode(std::shared_ptr<PnsNode> node) {
    node_ = std::move(node);
}

std::shared_ptr<PnsNode> PnsAlgoBase::getNode() const {
    return node_;
}

} // namespace ccad
