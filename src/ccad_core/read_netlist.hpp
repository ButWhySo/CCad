#ifndef CCAD_CORE_READ_NETLIST_HPP
#define CCAD_CORE_READ_NETLIST_HPP

#include <string>
#include <vector>

namespace ccad {

// Utility for parsing schematic netlists for footprint assignments (Cvpcb style).
class NetlistReader {
public:
    NetlistReader() = default;

    bool parse(const std::string& filepath);
};

} // namespace ccad

#endif // CCAD_CORE_READ_NETLIST_HPP
