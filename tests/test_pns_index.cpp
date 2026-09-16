#include "ccad_core/pns_node.hpp"
#include "ccad_core/pns_index.hpp"
#include "test_support.hpp"

int main() {
  ccad::PnsItem near;
  near.setPosition(10, 10, 2);
  ccad::PnsItem far;
  far.setPosition(100, 100);

  ccad::PnsIndex index;
  index.add(&near);
  index.add(&far);
  index.add(&near);

  const auto hits = index.query(12, 10, 0);
  require(hits.size() == 1 && hits.front() == &near,
          "PNS query includes item within its radius and deduplicates entries");
  require(index.query(0, 0, 1).empty(), "PNS query excludes distant items");
  require(index.query(0, 0, -1).empty(), "PNS query rejects negative search radius");

    ccad::PnsNode node;
    node.addItem(std::make_shared<ccad::PnsItem>(near));
    require(node.query(12, 10, 0).size() == 1, "PNS node forwards indexed query");
    node.clear();
    require(node.query(12, 10, 0).empty(), "PNS node query is empty after clear");
  return 0;
}
