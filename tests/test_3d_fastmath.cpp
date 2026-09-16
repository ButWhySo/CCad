#include "ccad_core/3d_math.hpp"

#include <cmath>
#include <cstdlib>
#include <iostream>

int main() {
  constexpr float pi = 3.14159265358979323846f;
  const float samples[] = {0.0f, 0.25f, pi / 2.0f, pi, -0.75f};
  for (float x : samples) {
    if (std::abs(ccad::FastMath3D::fastSin(x) - std::sin(x)) > 1e-6f ||
        std::abs(ccad::FastMath3D::fastCos(x) - std::cos(x)) > 1e-6f) {
      std::cerr << "FastMath3D trigonometric result mismatch\n";
      return EXIT_FAILURE;
    }
  }
  ccad::Math3D::Matrix4 transform;
  transform.m[0][0] = 2.0;
  transform.m[1][1] = 3.0;
  transform.m[2][2] = 4.0;
  transform.m[0][3] = 10.0;
  transform.m[1][3] = -2.0;
  const auto result = ccad::Math3D::transform({1.0, 2.0, 3.0}, transform);
  if (result.x != 12.0 || result.y != 4.0 || result.z != 12.0) {
    std::cerr << "Math3D affine transform mismatch\n";
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
