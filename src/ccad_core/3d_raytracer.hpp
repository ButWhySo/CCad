#ifndef CCAD_CORE_3D_RAYTRACER_HPP
#define CCAD_CORE_3D_RAYTRACER_HPP

#include "3d_scene_graph.hpp"

namespace ccad {

// Core engine for 3D raytracing and photorealistic rendering.
class Raytracer3D {
public:
    Raytracer3D() = default;

    void render(SceneGraph3D& scene);
};

} // namespace ccad

#endif // CCAD_CORE_3D_RAYTRACER_HPP
