#ifndef CCAD_CORE_DRAWING_TOOL_HPP
#define CCAD_CORE_DRAWING_TOOL_HPP

#include <string>

namespace ccad {

struct Board;

// Interactive drawing tool for geometric board primitives
class DrawingTool {
public:
    DrawingTool() = default;
    ~DrawingTool() = default;

    void setBoard(Board* board);

    void drawLine(double x1, double y1, double x2, double y2, int layer);
    void drawCircle(double cx, double cy, double radius, int layer);
    void drawText(const std::string& text, double x, double y, int layer);

private:
    Board* board_ = nullptr;
};

} // namespace ccad

#endif // CCAD_CORE_DRAWING_TOOL_HPP
