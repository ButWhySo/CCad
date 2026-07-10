#ifndef CCAD_CORE_NET_TIE_DRC_HPP
#define CCAD_CORE_NET_TIE_DRC_HPP

#include <string>
#include <vector>

namespace ccad {

class Board;

// Evaluates valid intentional shorting between different nets across specific footprint zones
class NetTieDrc {
public:
    explicit NetTieDrc(Board* board);
    ~NetTieDrc() = default;

    // Registers a footprint ID as a valid net tie boundary bridging two distinct net codes
    void registerNetTie(const std::string& footprintId, const std::string& netA, const std::string& netB);

    // Checks if a physical intersection between netA and netB is permitted at the given coordinate
    bool isIntersectionPermitted(const std::string& netA, const std::string& netB, double x, double y) const;

    // Evaluates all registered net ties to ensure they are physically connected by copper
    // Returns a list of net tie footprint IDs that are failing to bridge their defined nets
    std::vector<std::string> validateNetTies() const;

private:
    Board* board_ = nullptr;
    
    struct NetTieDef {
        std::string footprintId;
        std::string netA;
        std::string netB;
    };
    std::vector<NetTieDef> netTies_;
};

} // namespace ccad

#endif // CCAD_CORE_NET_TIE_DRC_HPP
