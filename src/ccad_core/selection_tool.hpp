#ifndef CCAD_CORE_SELECTION_TOOL_HPP
#define CCAD_CORE_SELECTION_TOOL_HPP

#include <vector>

namespace ccad {

class Board;
class BoardItem;

// Tool for managing selection of items on the PCB canvas
class SelectionTool {
public:
    SelectionTool() = default;
    ~SelectionTool() = default;

    void setBoard(Board* board);
    
    void selectPoint(double x, double y);
    void selectArea(double x1, double y1, double x2, double y2);
    void clearSelection();

    const std::vector<BoardItem*>& getSelection() const;

private:
    Board* board_ = nullptr;
    std::vector<BoardItem*> selection_;
};

} // namespace ccad

#endif // CCAD_CORE_SELECTION_TOOL_HPP
