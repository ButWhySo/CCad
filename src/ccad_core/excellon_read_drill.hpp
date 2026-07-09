#ifndef CCAD_CORE_EXCELLON_READ_DRILL_HPP
#define CCAD_CORE_EXCELLON_READ_DRILL_HPP

#include <string>
#include <vector>
#include "model.hpp"

namespace ccad {

// Represents an Excellon drill format file for CNC routing/drilling.
class ExcellonDrillFile {
public:
    struct DrillHole {
        Point position;
        Length diameter;
    };

    ExcellonDrillFile() = default;

    bool parse(const std::string& drill_data);
    
    const std::vector<DrillHole>& getHoles() const { return holes_; }

private:
    std::vector<DrillHole> holes_;
};

} // namespace ccad

#endif // CCAD_CORE_EXCELLON_READ_DRILL_HPP
