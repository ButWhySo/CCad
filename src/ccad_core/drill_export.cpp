#include "ccad_core/drill_export.hpp"
#include <sstream>
#include <map>
#include <vector>
#include <iomanip>
#include <cmath>

namespace ccad {

std::string exportToDrillExcellon(const Project& project) {
  std::stringstream ss;
  
  if (!project.board.has_value()) {
      return ss.str();
  }

  const Board& board = *project.board;

  // Excellon Format
  ss << "M48\n";
  ss << "INCH\n";

  // Group drills by size
  std::map<double, std::vector<Point>> drill_groups;

  for (const Via& via : board.vias) {
      double drill_in = (via.drill.nanometers / 1000000.0) / 25.4;
      drill_groups[drill_in].push_back(via.position);
  }

  for (const Pad& pad : board.pads) {
      if (pad.drill.has_value()) {
          double drill_in = (pad.drill->nanometers / 1000000.0) / 25.4;
          drill_groups[drill_in].push_back(pad.position);
      }
  }

  int tool_index = 1;
  std::map<double, int> tool_map;
  
  ss << std::fixed << std::setprecision(3);
  for (const auto& [size, points] : drill_groups) {
      // T1C0.015
      ss << "T" << tool_index << "C" << size << "\n";
      tool_map[size] = tool_index;
      tool_index++;
  }

  ss << "%\n";
  
  for (const auto& [size, points] : drill_groups) {
      ss << "T" << tool_map[size] << "\n";
      for (const Point& pt : points) {
          // Format as X01250Y01250 (Inches in 2.4 format)
          double x_in = (pt.x.nanometers / 1000000.0) / 25.4;
          double y_in = (pt.y.nanometers / 1000000.0) / 25.4;
          
          long x_fmt = std::lround(x_in * 10000.0);
          long y_fmt = std::lround(y_in * 10000.0);
          
          ss << "X" << std::setfill('0') << std::setw(6) << x_fmt 
             << "Y" << std::setfill('0') << std::setw(6) << y_fmt << "\n";
      }
  }

  ss << "M30\n";
  return ss.str();
}

} // namespace ccad
