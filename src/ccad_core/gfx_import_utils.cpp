#include "gfx_import_utils.hpp"

#include <iomanip>
#include <cstdlib>
#include <regex>
#include <sstream>

namespace ccad {

void convertImageToPolygons(const ImageImportData& img, Point pixel_scale, std::vector<SchGraphic>& out_graphics) {
    if (img.width == 0 || img.height == 0) {
        return;
    }
    const size_t row_size = static_cast<size_t>(img.width) * 4;
    if (img.width < 0 || img.height < 0 || img.pixels.size() < row_size * static_cast<size_t>(img.height)) {
        return;
    }
    for (int y = 0; y < img.height; ++y) {
        int x = 0;
        while (x < img.width) {
            const size_t offset = static_cast<size_t>(y) * row_size + static_cast<size_t>(x) * 4;
            const uint8_t alpha = img.pixels[offset + 3];
            if (img.has_alpha && alpha == 0) { ++x; continue; }
            const uint8_t r = img.pixels[offset];
            const uint8_t g = img.pixels[offset + 1];
            const uint8_t b = img.pixels[offset + 2];
            const int start_x = x++;
            while (x < img.width) {
                const size_t next = static_cast<size_t>(y) * row_size + static_cast<size_t>(x) * 4;
                if ((img.has_alpha && img.pixels[next + 3] == 0) || img.pixels[next] != r ||
                    img.pixels[next + 1] != g || img.pixels[next + 2] != b) break;
                ++x;
            }
            SchGraphic graphic;
            graphic.kind = "polygon";
            graphic.start = {Length(static_cast<int64_t>(start_x) * pixel_scale.x.nanometers),
                             Length(static_cast<int64_t>(y) * pixel_scale.y.nanometers)};
            graphic.end = {Length(static_cast<int64_t>(x) * pixel_scale.x.nanometers),
                           Length(static_cast<int64_t>(y + 1) * pixel_scale.y.nanometers)};
            graphic.points = {graphic.start,
                              {graphic.end.x, graphic.start.y},
                              graphic.end,
                              {graphic.start.x, graphic.end.y}};
            std::ostringstream color;
            color << '#' << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(r)
                  << std::setw(2) << static_cast<int>(g) << std::setw(2) << static_cast<int>(b);
            graphic.color = color.str();
            out_graphics.push_back(std::move(graphic));
        }
    }
}

void convertSVGToLibShapes(const std::string& svg_data, Point pixel_scale, Point offset, std::vector<SchGraphic>& out_graphics) { (void)svg_data; (void)pixel_scale; (void)offset; (void)out_graphics;
    if (svg_data.empty()) {
        return;
    }
    const auto number = [](const std::string& text, const std::string& key, double fallback = 0.0) {
        const std::regex attr("\\b" + key + "\\s*=\\s*[\\\"]([^\\\"]+)[\\\"]");
        std::smatch match;
        return std::regex_search(text, match, attr) ? std::strtod(match[1].str().c_str(), nullptr) : fallback;
    };
    const auto color = [](const std::string& text) {
        const std::regex attr("\\bfill\\s*=\\s*[\\\"]([^\\\"]+)[\\\"]");
        std::smatch match;
        return std::regex_search(text, match, attr) ? match[1].str() : std::string{};
    };
    const auto point = [&](double x, double y) {
        return Point{Length(offset.x.nanometers + static_cast<int64_t>(x * pixel_scale.x.nanometers)),
                     Length(offset.y.nanometers + static_cast<int64_t>(y * pixel_scale.y.nanometers))};
    };
    const std::regex element("<(line|rect|polygon|polyline)\\b([^>]*)/?>");
    for (auto it = std::sregex_iterator(svg_data.begin(), svg_data.end(), element); it != std::sregex_iterator(); ++it) {
        const std::string kind = (*it)[1].str();
        const std::string attrs = (*it)[2].str();
        SchGraphic graphic;
        graphic.kind = kind == "rect" ? "rectangle" : kind;
        graphic.color = color(attrs);
        if (kind == "line") {
            graphic.start = point(number(attrs, "x1"), number(attrs, "y1"));
            graphic.end = point(number(attrs, "x2"), number(attrs, "y2"));
            graphic.points = {graphic.start, graphic.end};
        } else if (kind == "rect") {
            graphic.start = point(number(attrs, "x"), number(attrs, "y"));
            graphic.end = point(number(attrs, "x") + number(attrs, "width"),
                                number(attrs, "y") + number(attrs, "height"));
            graphic.points = {graphic.start, {graphic.end.x, graphic.start.y}, graphic.end,
                              {graphic.start.x, graphic.end.y}};
        } else {
            const std::regex coordinate("(-?[0-9]+(?:\\.[0-9]+)?)\\s*,\\s*(-?[0-9]+(?:\\.[0-9]+)?)");
            for (auto point_it = std::sregex_iterator(attrs.begin(), attrs.end(), coordinate);
                 point_it != std::sregex_iterator(); ++point_it) {
                graphic.points.push_back(point(std::strtod((*point_it)[1].str().c_str(), nullptr),
                                               std::strtod((*point_it)[2].str().c_str(), nullptr)));
            }
            if (graphic.points.empty()) continue;
            graphic.start = graphic.points.front();
            graphic.end = graphic.points.back();
        }
        out_graphics.push_back(std::move(graphic));
    }
}

} // namespace ccad
