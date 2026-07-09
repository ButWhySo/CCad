#ifndef CCAD_CORE_X2_GERBER_ATTRIBUTES_HPP
#define CCAD_CORE_X2_GERBER_ATTRIBUTES_HPP

#include <string>
#include <map>

namespace ccad {

// Represents modern Gerber X2 metadata attributes for layers and parts.
class X2GerberAttributes {
public:
    X2GerberAttributes() = default;

    void addAttribute(const std::string& key, const std::string& value) {
        attributes_[key] = value;
    }
    
    std::string getAttribute(const std::string& key) const {
        auto it = attributes_.find(key);
        if (it != attributes_.end()) {
            return it->second;
        }
        return "";
    }

private:
    std::map<std::string, std::string> attributes_;
};

} // namespace ccad

#endif // CCAD_CORE_X2_GERBER_ATTRIBUTES_HPP
