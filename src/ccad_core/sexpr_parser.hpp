#pragma once

#include <memory>
#include <string>
#include <string_view>
#include <vector>
#include <stdexcept>
#include <cctype>

namespace ccad {

struct SExpr {
  std::string value;
  bool is_list = false;
  std::vector<std::unique_ptr<SExpr>> children;
};

inline const SExpr* findSExprChild(const SExpr* parent, const std::string& name) {
  if (!parent) return nullptr;
  for (const auto& child : parent->children) {
    if (child->is_list && !child->children.empty() && child->children[0]->value == name) {
      return child.get();
    }
  }
  return nullptr;
}

class SExprParser {
 public:
  explicit SExprParser(std::string_view source) : source_(source), pos_(0) {}

  std::unique_ptr<SExpr> parse() {
    skipWhitespace();
    if (pos_ >= source_.size()) return nullptr;
    if (source_[pos_] == '(') {
      return parseList();
    }
    return parseAtom();
  }

 private:
  std::unique_ptr<SExpr> parseList() {
    auto expr = std::make_unique<SExpr>();
    expr->is_list = true;
    expect('(');
    while (pos_ < source_.size()) {
      skipWhitespace();
      if (pos_ >= source_.size()) break;
      if (source_[pos_] == ')') {
        pos_++;
        break;
      }
      auto child = parse();
      if (child) {
        expr->children.push_back(std::move(child));
      }
    }
    return expr;
  }

  std::unique_ptr<SExpr> parseAtom() {
    auto expr = std::make_unique<SExpr>();
    if (pos_ >= source_.size()) return expr;
    if (source_[pos_] == '"') {
      expr->value = readQuoted();
    } else {
      const std::size_t start = pos_;
      while (pos_ < source_.size() && !std::isspace(static_cast<unsigned char>(source_[pos_])) &&
             source_[pos_] != '(' && source_[pos_] != ')') {
        ++pos_;
      }
      expr->value = std::string(source_.substr(start, pos_ - start));
    }
    return expr;
  }

  std::string readQuoted() {
    expect('"');
    std::string value;
    while (pos_ < source_.size()) {
      const char current = source_[pos_++];
      if (current == '"') {
        return value;
      }
      if (current == '\\') {
        if (pos_ >= source_.size()) {
          throw std::runtime_error("unterminated escape sequence");
        }
        value.push_back(source_[pos_++]);
      } else {
        value.push_back(current);
      }
    }
    throw std::runtime_error("unterminated quoted string");
  }

  void expect(char expected) {
    if (pos_ >= source_.size() || source_[pos_] != expected) {
      throw std::runtime_error(std::string("expected '") + expected + "'");
    }
    pos_++;
  }

  void skipWhitespace() {
    while (pos_ < source_.size() && std::isspace(static_cast<unsigned char>(source_[pos_]))) {
      ++pos_;
    }
  }

  std::string_view source_;
  std::size_t pos_ = 0;
};

inline std::unique_ptr<SExpr> parseSExpr(std::string_view source) {
  return SExprParser(source).parse();
}

}  // namespace ccad
