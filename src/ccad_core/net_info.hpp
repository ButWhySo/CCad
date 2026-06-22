#pragma once

#include <string>
#include <vector>
#include <cstdint>

#include "ccad_core/geometry.hpp"
#include "ccad_core/model.hpp"

namespace ccad {

struct NetInfoReport {
  std::string net_id;
  int pad_count = 0;
  int via_count = 0;
  int64_t track_length_nm = 0;
  
  bool has_bounding_box = false;
  int64_t bbox_min_x_nm = 0;
  int64_t bbox_min_y_nm = 0;
  int64_t bbox_max_x_nm = 0;
  int64_t bbox_max_y_nm = 0;

  std::vector<std::string> pending_kicad_features;
  std::vector<std::string> diagnostics;
};

// Computes pad/via counts, total track length, and bounding box for a net.
// Mapped from KiCad's NETINFO_ITEM capabilities.
NetInfoReport getNetInfo(const Project& project, const std::string& net_id);

}  // namespace ccad
