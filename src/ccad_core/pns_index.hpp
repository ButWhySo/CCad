#ifndef CCAD_CORE_PNS_INDEX_HPP
#define CCAD_CORE_PNS_INDEX_HPP

#include <vector>

namespace ccad {

class PnsItem;

// Spatial index for Push and Shove routing items.
class PnsIndex {
public:
    PnsIndex() = default;
    ~PnsIndex() = default;

    void add(PnsItem* item);
    void remove(PnsItem* item);
    void clear();

    std::vector<PnsItem*> query(int x, int y, int radius) const;

private:
    std::vector<PnsItem*> items_;
};

} // namespace ccad

#endif // CCAD_CORE_PNS_INDEX_HPP
