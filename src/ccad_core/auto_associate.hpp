#ifndef CCAD_CORE_AUTO_ASSOCIATE_HPP
#define CCAD_CORE_AUTO_ASSOCIATE_HPP

#include "cvpcb_listboxes.hpp"
#include <string>

#include "read_netlist.hpp"
#include <map>

namespace ccad {

// Heuristics for automatically assigning footprints to schematic symbols.
class AutoAssociate {
public:
    AutoAssociate() = default;

    void associate(const NetlistReader& reader);

    std::map<std::string, std::string> getAssociations() const;

private:
    std::map<std::string, std::string> associations_;
};

} // namespace ccad

#endif // CCAD_CORE_AUTO_ASSOCIATE_HPP
