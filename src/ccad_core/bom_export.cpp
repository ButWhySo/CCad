#include "ccad_core/bom_export.hpp"

#include <algorithm>
#include <cctype>
#include <sstream>
#include <stdexcept>
#include <vector>

namespace ccad {
namespace {

int compareNaturalIgnoreCase(const std::string& lhs, const std::string& rhs) {
  std::size_t left = 0;
  std::size_t right = 0;

  while (left < lhs.size() && right < rhs.size()) {
    const unsigned char left_char = static_cast<unsigned char>(lhs[left]);
    const unsigned char right_char = static_cast<unsigned char>(rhs[right]);

    if (std::isdigit(left_char) != 0 && std::isdigit(right_char) != 0) {
      std::size_t left_end = left;
      while (left_end < lhs.size() &&
             std::isdigit(static_cast<unsigned char>(lhs[left_end])) != 0) {
        ++left_end;
      }
      std::size_t right_end = right;
      while (right_end < rhs.size() &&
             std::isdigit(static_cast<unsigned char>(rhs[right_end])) != 0) {
        ++right_end;
      }

      std::size_t left_nonzero = left;
      while (left_nonzero < left_end && lhs[left_nonzero] == '0') {
        ++left_nonzero;
      }
      std::size_t right_nonzero = right;
      while (right_nonzero < right_end && rhs[right_nonzero] == '0') {
        ++right_nonzero;
      }

      const std::size_t left_digits = left_end - left_nonzero;
      const std::size_t right_digits = right_end - right_nonzero;
      if (left_digits != right_digits) {
        return left_digits < right_digits ? -1 : 1;
      }
      for (std::size_t offset = 0; offset < left_digits; ++offset) {
        if (lhs[left_nonzero + offset] != rhs[right_nonzero + offset]) {
          return lhs[left_nonzero + offset] < rhs[right_nonzero + offset] ? -1 : 1;
        }
      }
      const std::size_t left_run = left_end - left;
      const std::size_t right_run = right_end - right;
      if (left_run != right_run) {
        return left_run < right_run ? -1 : 1;
      }

      left = left_end;
      right = right_end;
      continue;
    }

    const char left_lower =
        static_cast<char>(std::tolower(static_cast<unsigned char>(lhs[left])));
    const char right_lower =
        static_cast<char>(std::tolower(static_cast<unsigned char>(rhs[right])));
    if (left_lower != right_lower) {
      return left_lower < right_lower ? -1 : 1;
    }
    ++left;
    ++right;
  }

  if (left == lhs.size() && right == rhs.size()) {
    return 0;
  }
  return left == lhs.size() ? -1 : 1;
}

std::string quotedSemicolonCsvField(const std::string& value) {
  std::string escaped;
  escaped.reserve(value.size() + 2);
  escaped.push_back('"');
  for (const char ch : value) {
    if (ch == '"') {
      escaped.push_back('"');
    }
    escaped.push_back(ch);
  }
  escaped.push_back('"');
  return escaped;
}

struct BoardBomEntry {
  std::vector<std::string> references;
  std::string value;
  std::string footprint_name;
  int count = 0;
};

}  // namespace

std::string exportToBomCsv(const Project& project) {
  std::stringstream ss;
  ss << "Designator,Part\n";

  const Schematic* schematic = primarySchematic(project);
  if (schematic == nullptr) {
    return ss.str();
  }

  // Create a copy of components to sort them by designator
  std::vector<Component> sorted_components = schematic->components;
  std::sort(sorted_components.begin(), sorted_components.end(),
            [](const Component& a, const Component& b) {
              return a.id < b.id;
            });

  for (const auto& comp : sorted_components) {
    // Basic CSV escaping logic if needed in the future, for now just print
    std::string id = comp.id;
    std::string part = comp.part;
    
    // Simple quotes for part if it contains comma
    if (part.find(',') != std::string::npos) {
        part = "\"" + part + "\"";
    }

    ss << id << "," << part << "\n";
  }

  return ss.str();
}

std::string exportBoardToBomCsv(const Project& project) {
  const Board* board = primaryBoard(project);
  if (board == nullptr || board->footprints.empty()) {
    throw std::runtime_error("Cannot export BOM: there are no footprints on the PCB.");
  }

  std::vector<BoardBomEntry> entries;
  for (const BoardFootprint& footprint : board->footprints) {
    if (footprint.exclude_from_bom) {
      continue;
    }

    bool found = false;
    for (BoardBomEntry& entry : entries) {
      if (entry.value == footprint.value && entry.footprint_name == footprint.footprint_name) {
        entry.references.push_back(footprint.reference);
        ++entry.count;
        found = true;
        break;
      }
    }

    if (!found) {
      entries.push_back(BoardBomEntry{
          .references = {footprint.reference},
          .value = footprint.value,
          .footprint_name = footprint.footprint_name,
          .count = 1,
      });
    }
  }

  for (BoardBomEntry& entry : entries) {
    std::sort(entry.references.begin(), entry.references.end(),
              [](const std::string& lhs, const std::string& rhs) {
                return compareNaturalIgnoreCase(lhs, rhs) < 0;
              });
  }
  std::sort(entries.begin(), entries.end(), [](const BoardBomEntry& lhs,
                                               const BoardBomEntry& rhs) {
    return compareNaturalIgnoreCase(lhs.references.front(), rhs.references.front()) < 0;
  });

  std::ostringstream out;
  out << "\"Id\";\"Designator\";\"Footprint\";\"Quantity\";\"Designation\";\"Supplier and ref\";\n";
  int id = 1;
  for (const BoardBomEntry& entry : entries) {
    std::ostringstream refs;
    for (std::size_t index = 0; index < entry.references.size(); ++index) {
      if (index != 0) {
        refs << ", ";
      }
      refs << entry.references.at(index);
    }

    out << id++ << ';' << quotedSemicolonCsvField(refs.str()) << ';'
        << quotedSemicolonCsvField(entry.footprint_name) << ';' << entry.count << ';'
        << quotedSemicolonCsvField(entry.value) << ";;;\n";
  }
  return out.str();
}

} // namespace ccad
