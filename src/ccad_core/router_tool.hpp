#ifndef CCAD_CORE_ROUTER_TOOL_HPP
#define CCAD_CORE_ROUTER_TOOL_HPP

namespace ccad {

struct Board;
class PnsRouter;

// Interactive router tool bridge mapping interactive/CLI actions to the PNS Router.
class RouterTool {
public:
    RouterTool() = default;
    ~RouterTool() = default;

    void setBoard(Board* board);
    void routeTrack(double x1, double y1, double x2, double y2);

    // Interactive UI hooks
    void startRouting(double x, double y, int layer);
    void updateRouting(double x, double y);
    void commitRouting();
    void cancelRouting();

private:
    Board* board_ = nullptr;
    bool routing_ = false;
    double start_x_ = 0.0, start_y_ = 0.0;
    double current_x_ = 0.0, current_y_ = 0.0;
    int layer_ = 0;
};

} // namespace ccad

#endif // CCAD_CORE_ROUTER_TOOL_HPP
