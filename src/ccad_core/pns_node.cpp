#include "pns_node.hpp"

namespace ccad {

void PnsNode::addItem(std::shared_ptr<PnsItem> item) {
    if (item) {
        items_.push_back(item);
    }
}

void PnsNode::clear() {
    items_.clear();
}

} // namespace ccad
