#ifndef CCAD_CORE_GERBER_FILE_IMAGE_LIST_HPP
#define CCAD_CORE_GERBER_FILE_IMAGE_LIST_HPP

#include "gerber_file_image.hpp"
#include <vector>
#include <memory>

namespace ccad {

// Manages a collection of parsed Gerber file images (e.g. for a full board project).
class GerberFileImageList {
public:
    GerberFileImageList() = default;

    void addImage(std::unique_ptr<GerberFileImage> image) {
        images_.push_back(std::move(image));
    }
    const std::vector<std::unique_ptr<GerberFileImage>>& getImages() const { return images_; }

private:
    std::vector<std::unique_ptr<GerberFileImage>> images_;
};

} // namespace ccad

#endif // CCAD_CORE_GERBER_FILE_IMAGE_LIST_HPP
