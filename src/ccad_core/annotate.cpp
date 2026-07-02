#include "annotate.hpp"
#include <algorithm>
#include <cctype>
#include <map>
#include <set>
#include <regex>

namespace ccad {

// Helper to extract the alphabetical prefix from a reference designator (e.g. "R1" -> "R", "U?" -> "U")
static std::string getReferencePrefix(const std::string& ref) {
  std::string prefix;
  for (char c : ref) {
    if (std::isalpha(c)) {
      prefix += c;
    } else {
      break; // Stop at first non-alpha
    }
  }
  return prefix.empty() ? "U" : prefix; // Default to U if no prefix
}

// Helper to extract the numerical suffix (if any)
static int getReferenceNumber(const std::string& ref) {
  std::string num_str;
  bool found_digit = false;
  for (char c : ref) {
    if (std::isdigit(c)) {
      num_str += c;
      found_digit = true;
    } else if (found_digit) {
      break;
    }
  }
  if (!num_str.empty()) {
    try {
      return std::stoi(num_str);
    } catch (...) {
      return -1;
    }
  }
  return -1;
}

void annotateSchematic(Schematic& schematic, const AnnotateOptions& options) {
  // 1. Gather all existing used numbers per prefix (if keeping existing)
  std::map<std::string, std::set<int>> used_numbers;
  
  if (options.algo == AnnotateAlgo::KeepExisting) {
    for (const auto& symbol : schematic.symbols) {
      if (symbol.reference.find('?') == std::string::npos) {
        int num = getReferenceNumber(symbol.reference);
        if (num > 0) {
          used_numbers[getReferencePrefix(symbol.reference)].insert(num);
        }
      }
    }
  } else if (options.algo == AnnotateAlgo::ResetAll) {
    for (auto& symbol : schematic.symbols) {
      std::string prefix = getReferencePrefix(symbol.reference);
      symbol.reference = prefix + "?";
    }
  }

  // 2. Identify symbols to annotate
  std::vector<SchSymbol*> to_annotate;
  for (auto& symbol : schematic.symbols) {
    if (options.algo == AnnotateAlgo::ResetAll || symbol.reference.find('?') != std::string::npos || getReferenceNumber(symbol.reference) <= 0) {
      to_annotate.push_back(&symbol);
    }
  }

  // 3. Sort symbols based on spatial options
  std::sort(to_annotate.begin(), to_annotate.end(), [&options](SchSymbol* a, SchSymbol* b) {
    // Sort logic mimics KiCad spatial sort (X then Y, or Y then X)
    long long ax = a->position.x.nanometers;
    long long ay = a->position.y.nanometers;
    long long bx = b->position.x.nanometers;
    long long by = b->position.y.nanometers;
    
    // Group into rows/columns (using a 100 mil tolerance in KiCad, which is 2540000 nm)
    const long long tol = 2540000;
    
    if (options.order == AnnotateOrder::SortX) {
      if (std::abs(ax - bx) > tol) return ax < bx;
      return ay < by;
    } else {
      if (std::abs(ay - by) > tol) return ay < by;
      return ax < bx;
    }
  });

  // 4. Assign numbers
  for (auto* symbol : to_annotate) {
    std::string prefix = getReferencePrefix(symbol->reference);
    auto& used = used_numbers[prefix];
    
    int next_num = options.start_number;
    while (used.count(next_num) > 0) {
      next_num++;
    }
    
    // Assign and mark used
    symbol->reference = prefix + std::to_string(next_num);
    used.insert(next_num);
  }
}

void annotateProject(Project& project, const AnnotateOptions& options) {
  // Iterate all schematics (a full project-level annotation would collect used numbers across all sheets,
  // but for CCad basic parity we annotate per schematic if the project is separated, or pass state down.
  // We will keep a global used_numbers map for the project.)
  
  std::map<std::string, std::set<int>> used_numbers;
  
  // First pass: collect or reset
  for (auto& sch : project.schematics) {
    if (options.algo == AnnotateAlgo::KeepExisting) {
      for (const auto& symbol : sch.symbols) {
        if (symbol.reference.find('?') == std::string::npos) {
          int num = getReferenceNumber(symbol.reference);
          if (num > 0) {
            used_numbers[getReferencePrefix(symbol.reference)].insert(num);
          }
        }
      }
    } else if (options.algo == AnnotateAlgo::ResetAll) {
      for (auto& symbol : sch.symbols) {
        std::string prefix = getReferencePrefix(symbol.reference);
        symbol.reference = prefix + "?";
      }
    }
  }
  
  // Second pass: gather, sort, and assign per schematic
  for (auto& sch : project.schematics) {
    std::vector<SchSymbol*> to_annotate;
    for (auto& symbol : sch.symbols) {
      if (options.algo == AnnotateAlgo::ResetAll || symbol.reference.find('?') != std::string::npos || getReferenceNumber(symbol.reference) <= 0) {
        to_annotate.push_back(&symbol);
      }
    }

    std::sort(to_annotate.begin(), to_annotate.end(), [&options](SchSymbol* a, SchSymbol* b) {
      long long ax = a->position.x.nanometers;
      long long ay = a->position.y.nanometers;
      long long bx = b->position.x.nanometers;
      long long by = b->position.y.nanometers;
      
      const long long tol = 2540000;
      
      if (options.order == AnnotateOrder::SortX) {
        if (std::abs(ax - bx) > tol) return ax < bx;
        return ay < by;
      } else {
        if (std::abs(ay - by) > tol) return ay < by;
        return ax < bx;
      }
    });

    for (auto* symbol : to_annotate) {
      std::string prefix = getReferencePrefix(symbol->reference);
      auto& used = used_numbers[prefix];
      
      int next_num = options.start_number;
      while (used.count(next_num) > 0) {
        next_num++;
      }
      
      symbol->reference = prefix + std::to_string(next_num);
      used.insert(next_num);
    }
  }
}

} // namespace ccad
