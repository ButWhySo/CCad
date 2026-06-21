#pragma once

#include "ccad_core/model.hpp"

#include <map>
#include <string>
#include <string_view>
#include <vector>

namespace ccad {

struct TextVariableReference {
  std::string name;
  bool resolved = false;
};

struct ExpandedBoardText {
  std::string id;
  std::string layer_id;
  std::string source_text;
  std::string expanded_text;
  std::vector<TextVariableReference> references;
};

std::vector<TextVariableReference> collectTextVariableReferences(
    std::string_view text, const std::map<std::string, std::string>& variables);

std::string expandTextVariables(std::string_view text,
                                const std::map<std::string, std::string>& variables,
                                std::vector<TextVariableReference>* references = nullptr);

std::vector<ExpandedBoardText> expandBoardTexts(const Project& project, const Board& board);

}  // namespace ccad
