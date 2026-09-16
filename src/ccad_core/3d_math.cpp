#include "3d_math.hpp"

namespace ccad {

Math3D::Vec3 Math3D::transform(const Vec3& v, const Matrix4& mat) {
    const double x = mat.m[0][0] * v.x + mat.m[0][1] * v.y + mat.m[0][2] * v.z + mat.m[0][3];
    const double y = mat.m[1][0] * v.x + mat.m[1][1] * v.y + mat.m[1][2] * v.z + mat.m[1][3];
    const double z = mat.m[2][0] * v.x + mat.m[2][1] * v.y + mat.m[2][2] * v.z + mat.m[2][3];
    const double w = mat.m[3][0] * v.x + mat.m[3][1] * v.y + mat.m[3][2] * v.z + mat.m[3][3];
    if (w == 0.0 || w == 1.0) return {x, y, z};
    return {x / w, y / w, z / w};
}

} // namespace ccad
