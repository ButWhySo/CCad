#ifndef CCAD_CORE_CVPCB_LISTBOXES_HPP
#define CCAD_CORE_CVPCB_LISTBOXES_HPP

#include <vector>
#include <string>

namespace ccad {

// Represents the data model for footprint assignment listboxes (Cvpcb style).
class CvpcbListboxes {
public:
    CvpcbListboxes() = default;

    void addComponent(const std::string& ref_des, const std::string& footprint) {
        assignments_.push_back({ref_des, footprint});
    }

    const std::vector<std::pair<std::string, std::string>>& getAssignments() const { return assignments_; }

private:
    std::vector<std::pair<std::string, std::string>> assignments_;
};

} // namespace ccad

#endif // CCAD_CORE_CVPCB_LISTBOXES_HPP
