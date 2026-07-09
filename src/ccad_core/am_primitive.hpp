#ifndef CCAD_CORE_AM_PRIMITIVE_HPP
#define CCAD_CORE_AM_PRIMITIVE_HPP

#include "am_param.hpp"
#include <vector>

namespace ccad {

// Represents a primitive drawing instruction in a Gerber aperture macro.
class AmPrimitive {
public:
    enum class Type {
        CIRCLE,
        VECTOR_LINE,
        CENTER_LINE,
        LOWER_LEFT_LINE,
        OUTLINE,
        POLYGON,
        MOIRE,
        THERMAL
    };

    AmPrimitive(Type type) : type_(type) {}

    Type getType() const { return type_; }
    
    void addParam(const AmParam& param) { params_.push_back(param); }
    const std::vector<AmParam>& getParams() const { return params_; }

private:
    Type type_;
    std::vector<AmParam> params_;
};

} // namespace ccad

#endif // CCAD_CORE_AM_PRIMITIVE_HPP
