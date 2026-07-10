#include "tool_state_control.hpp"

namespace ccad {

void ToolStateControl::setActiveTool(ToolId tool) {
    activeTool_ = tool;
}

ToolStateControl::ToolId ToolStateControl::getActiveTool() const {
    return activeTool_;
}

void ToolStateControl::cancelActiveTool() {
    activeTool_ = ToolId::None;
}

} // namespace ccad
