#include "kimath.hpp"
#include <cmath>

namespace ccad {

double KiMath::euclideanDistance(double x1, double y1, double x2, double y2) {
    return std::sqrt(std::pow(x2 - x1, 2) + std::pow(y2 - y1, 2));
}

double KiMath::normalizeAngle(double angle_degrees) {
    while (angle_degrees <= -180.0) angle_degrees += 360.0;
    while (angle_degrees > 180.0) angle_degrees -= 360.0;
    return angle_degrees;
}

} // namespace ccad
