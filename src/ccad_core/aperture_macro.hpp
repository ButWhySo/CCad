#ifndef CCAD_CORE_APERTURE_MACRO_HPP
#define CCAD_CORE_APERTURE_MACRO_HPP

#include "am_primitive.hpp"
#include <string>
#include <vector>

namespace ccad {

// Represents a Gerber aperture macro containing multiple primitives.
class ApertureMacro {
public:
    explicit ApertureMacro(const std::string& name) : name_(name) {}

    const std::string& getName() const { return name_; }

    void addPrimitive(const AmPrimitive& primitive) { primitives_.push_back(primitive); }
    const std::vector<AmPrimitive>& getPrimitives() const { return primitives_; }

private:
    std::string name_;
    std::vector<AmPrimitive> primitives_;
};

} // namespace ccad

#endif // CCAD_CORE_APERTURE_MACRO_HPP
