#ifndef CCAD_CORE_PAGELAYOUT_EDITOR_HPP
#define CCAD_CORE_PAGELAYOUT_EDITOR_HPP

#include <string>

namespace ccad {

// Utility for editing custom drawing sheets and title blocks.
class PagelayoutEditor {
public:
    PagelayoutEditor() = default;

    void loadTemplate(const std::string& filepath);

    std::string getTitle() const;
    void setTitle(const std::string& title);

    std::string getPaperSize() const;
    void setPaperSize(const std::string& size);

private:
    std::string title_;
    std::string paper_size_ = "A4";
};

} // namespace ccad

#endif // CCAD_CORE_PAGELAYOUT_EDITOR_HPP
