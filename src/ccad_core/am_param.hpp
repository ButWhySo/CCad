#ifndef CCAD_CORE_AM_PARAM_HPP
#define CCAD_CORE_AM_PARAM_HPP

#include <vector>
#include <string>

namespace ccad {

// Represents a parameter for a Gerber aperture macro.
struct AmParam {
    bool is_expression = false;
    double value = 0.0;
    std::string expression;
};

} // namespace ccad

#endif // CCAD_CORE_AM_PARAM_HPP
