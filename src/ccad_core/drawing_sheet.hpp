#ifndef CCAD_CORE_DRAWING_SHEET_HPP
#define CCAD_CORE_DRAWING_SHEET_HPP

#include <string>

namespace ccad {

// Core primitives for page layouts and title blocks.
class DrawingSheet {
public:
    DrawingSheet() = default;

    void setTitle(const std::string& title);
    std::string getTitle() const;
};

} // namespace ccad

#endif // CCAD_CORE_DRAWING_SHEET_HPP
