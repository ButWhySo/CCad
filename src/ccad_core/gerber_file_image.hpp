#ifndef CCAD_CORE_GERBER_FILE_IMAGE_HPP
#define CCAD_CORE_GERBER_FILE_IMAGE_HPP

#include "gerber_draw_item.hpp"
#include <string>
#include <vector>
#include <memory>

namespace ccad {

// Represents a parsed Gerber file containing a series of drawing items.
class GerberFileImage {
public:
    explicit GerberFileImage(const std::string& name) : name_(name) {}

    const std::string& getName() const { return name_; }

    void addItem(std::unique_ptr<GerberDrawItem> item) {
        items_.push_back(std::move(item));
    }
    const std::vector<std::unique_ptr<GerberDrawItem>>& getItems() const { return items_; }

private:
    std::string name_;
    std::vector<std::unique_ptr<GerberDrawItem>> items_;
};

} // namespace ccad

#endif // CCAD_CORE_GERBER_FILE_IMAGE_HPP
