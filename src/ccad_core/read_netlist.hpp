#ifndef CCAD_CORE_READ_NETLIST_HPP
#define CCAD_CORE_READ_NETLIST_HPP

#include <string>
#include <vector>

namespace ccad {

// Utility for parsing schematic netlists for footprint assignments (Cvpcb style).
class NetlistReader {
public:
    NetlistReader() = default;
    ~NetlistReader() = default;

    struct Component {
        std::string ref;
        std::string value;
        std::string footprint;
    };

    bool parse(const std::string& filepath);

    const std::vector<Component>& getComponents() const;

private:
    std::vector<Component> components_;
};

} // namespace ccad

#endif // CCAD_CORE_READ_NETLIST_HPP
