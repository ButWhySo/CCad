#include "3d_math.hpp"

#include <cmath>

namespace ccad {

float FastMath3D::fastSin(float x) {
    return std::sin(x);
}

float FastMath3D::fastCos(float x) {
    return std::cos(x);
}

} // namespace ccad
