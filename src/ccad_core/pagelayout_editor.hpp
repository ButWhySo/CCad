#ifndef CCAD_CORE_PAGELAYOUT_EDITOR_HPP
#define CCAD_CORE_PAGELAYOUT_EDITOR_HPP

#include <string>

namespace ccad {

// Utility for editing custom drawing sheets and title blocks.
class PagelayoutEditor {
public:
    PagelayoutEditor() = default;

    void loadTemplate(const std::string& filepath);
};

} // namespace ccad

#endif // CCAD_CORE_PAGELAYOUT_EDITOR_HPP
