#ifndef CCAD_CORE_ROUTER_TOOL_HPP
#define CCAD_CORE_ROUTER_TOOL_HPP

namespace ccad {

class Board;
class PnsRouter;

// Interactive router tool bridge mapping interactive/CLI actions to the PNS Router.
class RouterTool {
public:
    RouterTool() = default;
    ~RouterTool() = default;

    void setBoard(Board* board);
    void routeTrack(double x1, double y1, double x2, double y2);

private:
    Board* board_ = nullptr;
    // PnsRouter* router_ = nullptr; // Intentionally deferred logic
};

} // namespace ccad

#endif // CCAD_CORE_ROUTER_TOOL_HPP
