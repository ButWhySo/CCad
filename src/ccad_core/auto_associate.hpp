#ifndef CCAD_CORE_AUTO_ASSOCIATE_HPP
#define CCAD_CORE_AUTO_ASSOCIATE_HPP

#include "cvpcb_listboxes.hpp"
#include <string>

namespace ccad {

// Heuristics for automatically assigning footprints to schematic symbols.
class AutoAssociate {
public:
    AutoAssociate() = default;

    void associate(CvpcbListboxes& listboxes);
};

} // namespace ccad

#endif // CCAD_CORE_AUTO_ASSOCIATE_HPP
