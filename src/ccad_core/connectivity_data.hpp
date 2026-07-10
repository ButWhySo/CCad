#ifndef CCAD_CORE_CONNECTIVITY_DATA_HPP
#define CCAD_CORE_CONNECTIVITY_DATA_HPP

#include <vector>
#include <map>

namespace ccad {

class BoardItem;

// ConnectivityData manages the topological relationships (nets, ratsnest) of items on the board.
class ConnectivityData {
public:
    ConnectivityData() = default;
    ~ConnectivityData() = default;

    void build(const std::vector<BoardItem*>& items);
    void clear();

    int getNetCount() const;

private:
    int net_count_ = 0;
};

} // namespace ccad

#endif // CCAD_CORE_CONNECTIVITY_DATA_HPP
