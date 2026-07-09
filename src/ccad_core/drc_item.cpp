#include "drc_item.hpp"

namespace ccad {

DrcItem::DrcItem(int errorCode, const std::string& errorText)
    : error_code_(errorCode), error_text_(errorText) {
}

int DrcItem::getErrorCode() const {
    return error_code_;
}

std::string DrcItem::getErrorText() const {
    return error_text_;
}

} // namespace ccad
