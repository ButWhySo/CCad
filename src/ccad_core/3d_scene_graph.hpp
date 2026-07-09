#ifndef CCAD_CORE_3D_SCENE_GRAPH_HPP
#define CCAD_CORE_3D_SCENE_GRAPH_HPP

#include <vector>
#include <memory>
#include "3d_math.hpp"

namespace ccad {

class SceneNode3D {
public:
    virtual ~SceneNode3D() = default;

    void setTransform(const Math3D::Matrix4& mat) { transform_ = mat; }
    const Math3D::Matrix4& getTransform() const { return transform_; }

    void addChild(std::unique_ptr<SceneNode3D> child) {
        children_.push_back(std::move(child));
    }

private:
    Math3D::Matrix4 transform_;
    std::vector<std::unique_ptr<SceneNode3D>> children_;
};

// Manages the hierarchical 3D scene.
class SceneGraph3D {
public:
    SceneGraph3D() {
        root_ = std::make_unique<SceneNode3D>();
    }

    SceneNode3D* getRoot() { return root_.get(); }

private:
    std::unique_ptr<SceneNode3D> root_;
};

} // namespace ccad

#endif // CCAD_CORE_3D_SCENE_GRAPH_HPP
