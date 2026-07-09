#include "read_netlist.hpp"
#include "sexpr_parser.hpp"
#include <fstream>
#include <sstream>

namespace ccad {

bool NetlistReader::parse(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) return false;
    std::stringstream buffer;
    buffer << file.rdbuf();
    
    auto root = parseSExpr(buffer.str());
    if (!root || root->children.empty()) return false;
    
    // Stub for traversing (components (comp ...)) block
    return true;
}

const std::vector<NetlistReader::Component>& NetlistReader::getComponents() const {
    return components_;
}

} // namespace ccad
