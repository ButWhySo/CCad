#include "teardrop_generator.hpp"
#include "model.hpp"
#include "geometry.hpp"

#include <algorithm>
#include <cmath>
#include <map>
#include <string>
#include <stdexcept>
#include <utility>
#include <vector>

namespace ccad {

namespace {

double distanceMm(const Point& a, const Point& b) {
    return std::hypot(toMillimeters(a.x) - toMillimeters(b.x),
                      toMillimeters(a.y) - toMillimeters(b.y));
}

double effectivePadDiameterMm(const Pad& pad, const std::string& layer) {
    auto it = pad.padstack.copper_props.find(layer);
    if (it == pad.padstack.copper_props.end()) it = pad.padstack.copper_props.find("*.Cu");
    if (it == pad.padstack.copper_props.end()) return 0.0;
    const auto& shape = it->second.shape;
    // Offset/custom anchors require an actual shape intersection solver.
    if (shape.offset.x.nanometers || shape.offset.y.nanometers || shape.shape == PadShape::Custom)
        return 0.0;
    return std::min(toMillimeters(shape.size.width), toMillimeters(shape.size.height));
}

double clamp(double v, double cap) {
    return (cap > 0.0 && v > cap) ? cap : v;
}

void sampleBezier(std::vector<Point>& dst,
                  double x0, double y0, double x1, double y1,
                  double x2, double y2, double x3, double y3,
                  int n, bool includeEnd) {
    int cnt = includeEnd ? n + 1 : n;
    for (int i = 0; i < cnt; ++i) {
        double t  = static_cast<double>(i) / n;
        double mt = 1.0 - t;
        double mt2 = mt * mt, mt3 = mt2 * mt, t2 = t * t, t3 = t2 * t;
        dst.push_back({
            fromMillimeters(mt3*x0 + 3*mt2*t*x1 + 3*mt*t2*x2 + t3*x3),
            fromMillimeters(mt3*y0 + 3*mt2*t*y1 + 3*mt*t2*y2 + t3*y3)
        });
    }
}

// 5-point teardrop. track_dir points FROM centre TOWARD track.
// A/B = track side, C/D/E = pad arc + far tip.
std::vector<Point> buildOutline(const Point& centre,
                                double dx, double dy,  // unit track_dir
                                double effD, double trackW, double availableLength,
                                const TeardropGenerator::TeardropSettings& s) {
    double tdLen  = std::min(clamp(effD * s.lengthRatio, s.maxLengthMm), availableLength);
    double tdWide = std::min(effD, clamp(effD * s.widthRatio, s.maxWidthMm));
    double halfW  = tdWide / 2.0;
    double radius = effD  / 2.0;
    if (tdLen < 1e-9 || tdWide <= trackW) return {};

    double px = -dy, py = dx;  // perpendicular (left)
    double cx = toMillimeters(centre.x), cy = toMillimeters(centre.y);

    double Cx = cx + px*halfW,        Cy = cy + py*halfW;
    double Dx = cx - dx*radius,       Dy = cy - dy*radius;
    double Ex = cx - px*halfW,        Ey = cy - py*halfW;
    double Ax = cx + px*trackW/2 + dx*tdLen, Ay = cy + py*trackW/2 + dy*tdLen;
    double Bx = cx - px*trackW/2 + dx*tdLen, By = cy - py*trackW/2 + dy*tdLen;

    std::vector<Point> out;

    if (!s.curvedEdges) {
        out.push_back({fromMillimeters(Cx), fromMillimeters(Cy)});
        out.push_back({fromMillimeters(Dx), fromMillimeters(Dy)});
        out.push_back({fromMillimeters(Ex), fromMillimeters(Ey)});
        out.push_back({fromMillimeters(Bx), fromMillimeters(By)});
        out.push_back({fromMillimeters(Ax), fromMillimeters(Ay)});
        return out;
    }

    // Monotone cubic taper; bounded by the same straight envelope.
    int seg = std::max(3, std::min(10, s.curveSegments));
    double tAx = Ax - dx*tdLen/3, tAy = Ay - dy*tdLen/3;
    double tCx = Cx + dx*tdLen/3, tCy = Cy + dy*tdLen/3;
    sampleBezier(out, Ax, Ay, tAx, tAy, tCx, tCy, Cx, Cy, seg, false);

    out.push_back({fromMillimeters(Cx), fromMillimeters(Cy)});
    out.push_back({fromMillimeters(Dx), fromMillimeters(Dy)});
    out.push_back({fromMillimeters(Ex), fromMillimeters(Ey)});

    double tEx = Ex + dx*tdLen/3, tEy = Ey + dy*tdLen/3;
    double tBx = Bx - dx*tdLen/3, tBy = By - dy*tdLen/3;
    out.pop_back(); // sampleBezier includes E; keep no duplicate edge.
    sampleBezier(out, Ex, Ey, tEx, tEy, tBx, tBy, Bx, By, seg, false);

    out.push_back({fromMillimeters(Bx), fromMillimeters(By)});
    return out;
}

void push(Board* board,
          const Point& centre, double dx, double dy,
          double effD, double trackW,
          const std::string& netId, const std::string& layerId,
          const std::string& trackId,
          const std::string& padId, const std::string& viaId,
          const TeardropGenerator::TeardropSettings& s,
          int& counter) {
    if (effD <= 0.0 || trackW <= 0.0 || netId.empty() || (trackW / effD) >= s.widthFilterRatio) return;
    for (const auto& existing : board->teardrops) {
        if (existing.locked && existing.anchor_track_id == trackId &&
            existing.anchor_pad_id == padId && existing.anchor_via_id == viaId) return;
    }
    double availableLength = 0.0;
    for (const auto& track : board->tracks) if (track.id == trackId) {
        availableLength = distanceMm(track.start, track.end);
        break;
    }
    auto outline = buildOutline(centre, dx, dy, effD, trackW, availableLength, s);
    if (outline.empty()) return;
    BoardTeardrop td;
    do {
        td.id = "teardrop_" + std::to_string(++counter);
    } while (std::any_of(board->teardrops.begin(), board->teardrops.end(),
                        [&](const auto& old) { return old.id == td.id; }));
    td.net_id          = netId;
    td.layer_id        = layerId;
    td.anchor_pad_id   = padId;
    td.anchor_via_id   = viaId;
    td.anchor_track_id = trackId;
    td.outline         = std::move(outline);
    board->teardrops.push_back(std::move(td));
}

}  // namespace

TeardropGenerator::TeardropGenerator(Board* board) : board_(board) {}

void TeardropGenerator::setSettings(const TeardropSettings& s) {
    for (double value : {s.lengthRatio, s.widthRatio, s.maxLengthMm, s.maxWidthMm,
                         s.widthFilterRatio, s.connectionToleranceMm}) {
        if (!std::isfinite(value) || value < 0.0)
            throw std::invalid_argument("teardrop settings must be finite and non-negative");
    }
    if (s.widthRatio > 1.0 || s.widthFilterRatio > 1.0)
        throw std::invalid_argument("teardrop width ratios must not exceed one");
    settings_ = s;
}
TeardropGenerator::TeardropSettings TeardropGenerator::getSettings() const { return settings_; }

bool TeardropGenerator::generateTeardrops() {
    if (!board_ || !settings_.enabled) return false;
    std::erase_if(board_->teardrops, [](const auto& td) { return !td.locked; });

    int n = 0;
    const double tol = settings_.connectionToleranceMm;

    for (const TrackSegment& tr : board_->tracks) {
        double tx = toMillimeters(tr.end.x) - toMillimeters(tr.start.x);
        double ty = toMillimeters(tr.end.y) - toMillimeters(tr.start.y);
        double tlen = std::hypot(tx, ty);
        if (tlen < 1e-9) continue;
        double ux = tx / tlen, uy = ty / tlen;
        double tw = toMillimeters(tr.width);
        if (!board_->layers.empty() && !std::any_of(board_->layers.begin(), board_->layers.end(),
            [&](const auto& layer) { return layer.id == tr.layer_id && layer.kind == "copper"; })) continue;

        for (const Pad& p : board_->pads) {
            if (!p.teardrops_enabled || p.net_id != tr.net_id) continue;
            bool smd = (p.type == "smd");
            if ( smd && !settings_.targetSMDPads) continue;
            if (!smd && !settings_.targetPTHPads)  continue;
            double effD = effectivePadDiameterMm(p, tr.layer_id);
            if (distanceMm(tr.start, p.position) <= tol)
                push(board_, p.position,  ux,  uy, effD, tw, p.net_id, tr.layer_id, tr.id, p.id, "", settings_, n);
            if (distanceMm(tr.end,   p.position) <= tol)
                push(board_, p.position, -ux, -uy, effD, tw, p.net_id, tr.layer_id, tr.id, p.id, "", settings_, n);
        }

        if (settings_.targetVias) {
            for (const Via& v : board_->vias) {
                if (!v.teardrops_enabled || v.net_id != tr.net_id) continue;
                double effD = toMillimeters(v.diameter);
                if (effD <= 0.0) continue;
                if (distanceMm(tr.start, v.position) <= tol)
                    push(board_, v.position,  ux,  uy, effD, tw, v.net_id, tr.layer_id, tr.id, "", v.id, settings_, n);
                if (distanceMm(tr.end,   v.position) <= tol)
                    push(board_, v.position, -ux, -uy, effD, tw, v.net_id, tr.layer_id, tr.id, "", v.id, settings_, n);
            }
        }
    }

    if (settings_.targetTrack2Track) {
        double grid = std::max(tol, 0.001);
        auto Q = [&](double mm) { return static_cast<long long>(std::round(mm / grid)); };
        auto K = [&](const Point& p) { return std::make_pair(Q(toMillimeters(p.x)), Q(toMillimeters(p.y))); };

        struct EP { std::size_t idx; bool isEnd; };
        std::map<std::pair<long long, long long>, std::vector<EP>> epMap;

        for (std::size_t i = 0; i < board_->tracks.size(); ++i) {
            const auto& t = board_->tracks[i];
            epMap[K(t.start)].push_back({i, false});
            epMap[K(t.end  )].push_back({i, true});
        }

        for (auto& [key, eps] : epMap) {
            if (eps.size() != 2) continue;
            const auto& tA = board_->tracks[eps[0].idx];
            const auto& tB = board_->tracks[eps[1].idx];
            if (tA.layer_id != tB.layer_id || tA.net_id != tB.net_id) continue;
            double wA = toMillimeters(tA.width), wB = toMillimeters(tB.width);
            if (std::abs(wA - wB) < 0.001) continue;

            bool aWider = (wA > wB);
            const auto& wide   = aWider ? tA : tB;
            const auto& narrow = aWider ? tB : tA;
            bool narrowEnd = aWider ? eps[1].isEnd : eps[0].isEnd;
            double wW          = aWider ? wA : wB;
            double nW          = aWider ? wB : wA;
            if ((nW / wW) >= settings_.widthFilterRatio) continue;

            const Point& junc = narrowEnd ? narrow.end : narrow.start;
            double wx = toMillimeters(narrow.end.x) - toMillimeters(narrow.start.x);
            double wy = toMillimeters(narrow.end.y) - toMillimeters(narrow.start.y);
            double wl = std::hypot(wx, wy);
            if (wl < 1e-9) continue;
            wx /= wl; wy /= wl;
            double ddx = narrowEnd ? -wx : wx;
            double ddy = narrowEnd ? -wy : wy;

            push(board_, junc, ddx, ddy, wW, nW,
                 wide.net_id, wide.layer_id, narrow.id, "", "", settings_, n);
        }
    }

    return (n > 0);
}

void TeardropGenerator::removeTeardrops() {
    if (board_) board_->teardrops.clear();
}

}  // namespace ccad
