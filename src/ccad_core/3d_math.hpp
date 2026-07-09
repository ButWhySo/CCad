#ifndef CCAD_CORE_3D_MATH_HPP
#define CCAD_CORE_3D_MATH_HPP

namespace ccad {

// Represents 3D coordinate transformations and utilities.
class Math3D {
public:
    struct Vec3 {
        double x = 0.0;
        double y = 0.0;
        double z = 0.0;
    };

    struct Matrix4 {
        double m[4][4] = {
            {1, 0, 0, 0},
            {0, 1, 0, 0},
            {0, 0, 1, 0},
            {0, 0, 0, 1}
        };
    };

    static Vec3 transform(const Vec3& v, const Matrix4& mat);
};

// Fast mathematical approximations for 3D operations.
class FastMath3D {
public:
    static float fastSin(float x);
    static float fastCos(float x);
};

} // namespace ccad

#endif // CCAD_CORE_3D_MATH_HPP
