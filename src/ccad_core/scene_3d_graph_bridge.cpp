#include "scene_3d_graph_bridge.hpp"
#include "board.hpp"

namespace ccad {

Scene3DGraphBridge::Scene3DGraphBridge(Board* board)
    : board_(board) {
}

void Scene3DGraphBridge::buildScene() {
    if (!board_) return;

    clearScene();

    // Stub:
    // 1. Traverse board outline, calculate total thickness from stackup, emit "BoardBody" node
    // 2. Traverse tracks and zones per layer, offset Z by layer stackup position, emit copper nodes
    // 3. Traverse footprints, resolve Model3DRegistry transforms, emit "FootprintModel" nodes
    
    Scene3DNode dummyNode;
    dummyNode.id = "board_body_001";
    dummyNode.type = "BoardBody";
    nodes_.push_back(dummyNode);
}

std::vector<Scene3DNode> Scene3DGraphBridge::getNodes() const {
    return nodes_;
}

void Scene3DGraphBridge::clearScene() {
    nodes_.clear();
}

} // namespace ccad
