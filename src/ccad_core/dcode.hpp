#ifndef CCAD_CORE_DCODE_HPP
#define CCAD_CORE_DCODE_HPP

#include "am_primitive.hpp"

namespace ccad {

// Represents a Gerber D-Code tool definition.
class DCode {
public:
    enum class Shape {
        CIRCLE,
        RECT,
        OVAL,
        POLYGON,
        MACRO
    };

    explicit DCode(int code, Shape shape = Shape::CIRCLE) 
        : code_(code), shape_(shape) {}

    int getCode() const { return code_; }
    Shape getShape() const { return shape_; }
    
    // Size parameters in generic units
    void setSize(double width, double height = 0.0) {
        width_ = width;
        height_ = height > 0 ? height : width;
    }

private:
    int code_;
    Shape shape_;
    double width_ = 0.0;
    double height_ = 0.0;
};

} // namespace ccad

#endif // CCAD_CORE_DCODE_HPP
