#include "ccad_core/fix_board_shape.hpp"
#include "ccad_core/nanoflann.hpp"
#include <set>
#include <algorithm>
#include <limits>

namespace ccad {

struct BoardGraphicEndpointsAdaptor {
    std::vector<std::pair<Point, BoardGraphic*>> endpoints;

    BoardGraphicEndpointsAdaptor(const std::vector<BoardGraphic*>& graphics) {
        endpoints.reserve(graphics.size() * 2);
        for (BoardGraphic* graphic : graphics) {
            if (!graphic->locked) {
                endpoints.emplace_back(graphic->start, graphic);
                endpoints.emplace_back(graphic->end, graphic);
            }
        }
    }

    size_t kdtree_get_point_count() const { return endpoints.size(); }

    double kdtree_get_pt(const size_t idx, const size_t dim) const {
        if (dim == 0)
            return static_cast<double>(endpoints[idx].first.x.nanometers);
        else
            return static_cast<double>(endpoints[idx].first.y.nanometers);
    }

    template <class BBOX>
    bool kdtree_get_bbox(BBOX&) const { return false; }
};

using KDTree = nanoflann::KDTreeSingleIndexAdaptor<
    nanoflann::L2_Simple_Adaptor<double, BoardGraphicEndpointsAdaptor>,
    BoardGraphicEndpointsAdaptor,
    2>;

static double distanceSq(const Point& a, const Point& b) {
    double dx = static_cast<double>(a.x.nanometers) - static_cast<double>(b.x.nanometers);
    double dy = static_cast<double>(a.y.nanometers) - static_cast<double>(b.y.nanometers);
    return dx * dx + dy * dy;
}

static BoardGraphic* findNext(BoardGraphic* current, const Point& pt, const KDTree& kdTree,
                              const BoardGraphicEndpointsAdaptor& adaptor, double epsilon_sq,
                              const std::set<BoardGraphic*>& available) {
    const double query_pt[2] = {static_cast<double>(pt.x.nanometers), static_cast<double>(pt.y.nanometers)};
    
    uint32_t indices[2];
    double distances[2];
    
    kdTree.knnSearch(query_pt, 2, indices, distances);

    if (distances[0] == std::numeric_limits<double>::max())
        return nullptr;

    BoardGraphic* closest_graphic = nullptr;
    double closest_dist_sq = epsilon_sq;

    for (size_t i = 0; i < 2; ++i) {
        if (distances[i] == std::numeric_limits<double>::max())
            continue;

        BoardGraphic* candidate = adaptor.endpoints[indices[i]].second;

        if (candidate == current)
            continue;

        if (available.find(candidate) == available.end())
            continue;

        if (distances[i] <= closest_dist_sq) {
            closest_dist_sq = distances[i];
            closest_graphic = candidate;
        }
    }

    return closest_graphic;
}

static void snapEndpoints(BoardGraphic* a, BoardGraphic* b) {
    double d[4] = {
        distanceSq(a->start, b->start),
        distanceSq(a->start, b->end),
        distanceSq(a->end, b->start),
        distanceSq(a->end, b->end)
    };

    int min_idx = 0;
    for (int i = 1; i < 4; ++i) {
        if (d[i] < d[min_idx]) {
            min_idx = i;
        }
    }

    auto midpoint = [](const Point& p1, const Point& p2) -> Point {
        return { nanometers((p1.x.nanometers + p2.x.nanometers) / 2),
                 nanometers((p1.y.nanometers + p2.y.nanometers) / 2) };
    };

    Point mid;
    if (min_idx == 0) { mid = midpoint(a->start, b->start); a->start = mid; b->start = mid; }
    else if (min_idx == 1) { mid = midpoint(a->start, b->end); a->start = mid; b->end = mid; }
    else if (min_idx == 2) { mid = midpoint(a->end, b->start); a->end = mid; b->start = mid; }
    else if (min_idx == 3) { mid = midpoint(a->end, b->end); a->end = mid; b->end = mid; }
}

void connectBoardShapes(std::vector<BoardGraphic*>& graphics, Length epsilon) {
    if (graphics.empty()) return;

    BoardGraphicEndpointsAdaptor adaptor(graphics);
    if (adaptor.endpoints.empty()) return;

    // build kd-tree
    KDTree kdTree(2, adaptor);
    kdTree.buildIndex();

    double eps_nm = static_cast<double>(epsilon.nanometers);
    double epsilon_sq = eps_nm * eps_nm;

    std::set<BoardGraphic*> available;
    for (BoardGraphic* g : graphics) {
        if (!g->locked) available.insert(g);
    }
    std::set<BoardGraphic*> startCandidates = available;

    while (!startCandidates.empty()) {
        BoardGraphic* start_graphic = *startCandidates.begin();

        auto walkFrom = [&](BoardGraphic* curr, Point startPt) {
            Point prevPt = startPt;
            for (;;) {
                BoardGraphic* nextGraphic = findNext(curr, prevPt, kdTree, adaptor, epsilon_sq, available);
                if (!nextGraphic) break;

                snapEndpoints(curr, nextGraphic);

                double dist_to_start = distanceSq(prevPt, nextGraphic->start);
                double dist_to_end = distanceSq(prevPt, nextGraphic->end);
                prevPt = (dist_to_start < dist_to_end) ? nextGraphic->end : nextGraphic->start;
                
                curr = nextGraphic;
                available.erase(curr);
                startCandidates.erase(curr);
            }
        };

        const Point ptEnd = start_graphic->end;
        const Point ptStart = start_graphic->start;

        BoardGraphic* grAtEnd = findNext(start_graphic, ptEnd, kdTree, adaptor, epsilon_sq, available);
        BoardGraphic* grAtStart = findNext(start_graphic, ptStart, kdTree, adaptor, epsilon_sq, available);

        bool beginFromEndPt = true;
        if (grAtEnd && grAtStart) {
            double dAtEnd = std::min(distanceSq(ptEnd, grAtEnd->start), distanceSq(ptEnd, grAtEnd->end));
            double dAtStart = std::min(distanceSq(ptStart, grAtStart->start), distanceSq(ptStart, grAtStart->end));
            beginFromEndPt = (dAtEnd <= dAtStart);
        } else if (grAtStart) {
            beginFromEndPt = false;
        }

        if (beginFromEndPt) {
            walkFrom(start_graphic, start_graphic->end);
            walkFrom(start_graphic, start_graphic->start);
        } else {
            walkFrom(start_graphic, start_graphic->start);
            walkFrom(start_graphic, start_graphic->end);
        }
        
        startCandidates.erase(start_graphic);
    }
}

} // namespace ccad
