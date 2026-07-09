#ifndef CCAD_CORE_DRC_ITEM_HPP
#define CCAD_CORE_DRC_ITEM_HPP

#include <string>

namespace ccad {

// Strongly typed Design Rule Check violation item.
class DrcItem {
public:
    DrcItem(int errorCode, const std::string& errorText);
    ~DrcItem() = default;

    int getErrorCode() const;
    std::string getErrorText() const;

private:
    int error_code_;
    std::string error_text_;
};

} // namespace ccad

#endif // CCAD_CORE_DRC_ITEM_HPP
