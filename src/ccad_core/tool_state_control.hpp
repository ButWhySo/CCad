#ifndef CCAD_CORE_TOOL_STATE_CONTROL_HPP
#define CCAD_CORE_TOOL_STATE_CONTROL_HPP

namespace ccad {

// Controls state machines of currently active interactive tools
class ToolStateControl {
public:
    ToolStateControl() = default;
    ~ToolStateControl() = default;

    enum class ToolId {
        None,
        Selection,
        Routing,
        Drawing,
        Placement
    };

    void setActiveTool(ToolId tool);
    ToolId getActiveTool() const;

    void cancelActiveTool();

private:
    ToolId activeTool_ = ToolId::None;
};

} // namespace ccad

#endif // CCAD_CORE_TOOL_STATE_CONTROL_HPP
