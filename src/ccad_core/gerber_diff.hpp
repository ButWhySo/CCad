#ifndef CCAD_CORE_GERBER_DIFF_HPP
#define CCAD_CORE_GERBER_DIFF_HPP

#include "gerber_file_image.hpp"

namespace ccad {

// Utility for comparing two Gerber documents for differences.
class GerberDiff {
public:
    GerberDiff() = default;

    bool compare(const GerberFileImage& a, const GerberFileImage& b);
};

} // namespace ccad

#endif // CCAD_CORE_GERBER_DIFF_HPP
