#include "ccad_core/board_text_var_adapter.hpp"

#include <cstddef>
#include <utility>

namespace ccad {
namespace {

constexpr std::string_view kOpen = "${";

bool isValidVariableName(std::string_view name) {
  return !name.empty();
}

}  // namespace

std::vector<TextVariableReference> collectTextVariableReferences(
    std::string_view text, const std::map<std::string, std::string>& variables) {
  std::vector<TextVariableReference> references;
  std::size_t cursor = 0;

  while (cursor < text.size()) {
    const std::size_t open = text.find(kOpen, cursor);
    if (open == std::string_view::npos) {
      break;
    }

    const std::size_t name_begin = open + kOpen.size();
    const std::size_t close = text.find('}', name_begin);
    if (close == std::string_view::npos) {
      break;
    }

    const std::string name(text.substr(name_begin, close - name_begin));
    if (isValidVariableName(name)) {
      references.push_back(TextVariableReference{.name = name,
                                                 .resolved = variables.contains(name)});
    }
    cursor = close + 1;
  }

  return references;
}

std::string expandTextVariables(std::string_view text,
                                const std::map<std::string, std::string>& variables,
                                std::vector<TextVariableReference>* references) {
  std::string expanded;
  expanded.reserve(text.size());

  std::vector<TextVariableReference> found;
  std::size_t cursor = 0;

  while (cursor < text.size()) {
    const std::size_t open = text.find(kOpen, cursor);
    if (open == std::string_view::npos) {
      expanded.append(text.substr(cursor));
      break;
    }

    expanded.append(text.substr(cursor, open - cursor));

    const std::size_t name_begin = open + kOpen.size();
    const std::size_t close = text.find('}', name_begin);
    if (close == std::string_view::npos) {
      expanded.append(text.substr(open));
      break;
    }

    const std::string name(text.substr(name_begin, close - name_begin));
    if (!isValidVariableName(name)) {
      expanded.append(text.substr(open, close - open + 1));
      cursor = close + 1;
      continue;
    }

    const auto variable = variables.find(name);
    const bool resolved = variable != variables.end();
    found.push_back(TextVariableReference{.name = name, .resolved = resolved});

    if (resolved) {
      expanded.append(variable->second);
    } else {
      expanded.append(text.substr(open, close - open + 1));
    }

    cursor = close + 1;
  }

  if (references) {
    *references = std::move(found);
  }

  return expanded;
}

std::vector<ExpandedBoardText> expandBoardTexts(const Project& project, const Board& board) {
  std::vector<ExpandedBoardText> texts;
  texts.reserve(board.texts.size());

  for (const BoardText& text : board.texts) {
    std::vector<TextVariableReference> references;
    const std::string expanded =
        expandTextVariables(text.text, project.text_variables, &references);
    texts.push_back(ExpandedBoardText{.id = text.id,
                                      .layer_id = text.layer_id,
                                      .source_text = text.text,
                                      .expanded_text = expanded,
                                      .references = std::move(references)});
  }

  return texts;
}

}  // namespace ccad
