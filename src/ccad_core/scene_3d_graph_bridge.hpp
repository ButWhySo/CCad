#ifndef CCAD_CORE_SCENE_3D_GRAPH_BRIDGE_HPP
#define CCAD_CORE_SCENE_3D_GRAPH_BRIDGE_HPP

#include <vector>
#include <string>

namespace ccad {

struct Board;

// Represents a bounding volume and rendering parameters for a 3D PCB feature
struct Scene3DNode {
    std::string id;
    std::string type; // e.g., "BoardBody", "CopperTrace", "FootprintModel"
    double boundsMinX = 0.0, boundsMinY = 0.0, boundsMinZ = 0.0;
    double boundsMaxX = 0.0, boundsMaxY = 0.0, boundsMaxZ = 0.0;
    // ... Material / color properties could go here
};

// Translates 2D board geometry and stackup into a flattened 3D scene representation
class Scene3DGraphBridge {
public:
    explicit Scene3DGraphBridge(Board* board);
    ~Scene3DGraphBridge() = default;

    // Traverses the board and builds the list of 3D scene nodes required for rendering
    void buildScene();

    // Retrieves the constructed nodes
    std::vector<Scene3DNode> getNodes() const;

    // Clears the internal scene representation
    void clearScene();

private:
    Board* board_ = nullptr;
    std::vector<Scene3DNode> nodes_;
};

} // namespace ccad

#endif // CCAD_CORE_SCENE_3D_GRAPH_BRIDGE_HPP
