#ifndef CCAD_CORE_AM_EVALUATE_HPP
#define CCAD_CORE_AM_EVALUATE_HPP

#include "am_param.hpp"
#include <map>
#include <string>

namespace ccad {

// Evaluates Gerber aperture macro mathematical expressions.
class AmEvaluate {
public:
    AmEvaluate() = default;
    
    void setVariable(const std::string& name, double value) {
        variables_[name] = value;
    }

    double evaluate(const std::string& expression) const;

private:
    std::map<std::string, double> variables_;
};

} // namespace ccad

#endif // CCAD_CORE_AM_EVALUATE_HPP
