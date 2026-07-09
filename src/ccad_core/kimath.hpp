#ifndef CCAD_CORE_KIMATH_HPP
#define CCAD_CORE_KIMATH_HPP

namespace ccad {

// Core math geometry helpers for CAD coordinate calculations.
class KiMath {
public:
    static double euclideanDistance(double x1, double y1, double x2, double y2);
    static double normalizeAngle(double angle_degrees);
};

} // namespace ccad

#endif // CCAD_CORE_KIMATH_HPP
