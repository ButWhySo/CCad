#pragma once

#include <QFont>
#include <QTextBrowser>
#include <QTextCharFormat>
#include <QTextCursor>
#include <QTextDocument>
#include <QTextDocumentFragment>
#include <QUrl>
#include <QVariant>

#include <functional>
#include <utility>

class AgentChatBrowser final : public QTextBrowser {
 public:
  using SafeLinkHandler = std::function<void(const QUrl&)>;

  explicit AgentChatBrowser(QWidget* parent = nullptr) : QTextBrowser(parent) {
    setOpenLinks(false);
    setOpenExternalLinks(false);
    QObject::connect(this, &QTextBrowser::anchorClicked, this,
                     [this](const QUrl& url) {
                       if (safe_link_handler_ && isSafeExternalLink(url)) {
                         safe_link_handler_(url);
                       }
                     });
  }

  void setSafeLinkHandler(SafeLinkHandler handler) {
    safe_link_handler_ = std::move(handler);
  }

  static bool assistantMessageUsesMarkdown(const QString& content_format) {
    return content_format != QStringLiteral("plain");
  }

  static bool isSafeExternalLink(const QUrl& url) {
    const QString scheme = url.scheme().toLower();
    return (scheme == QStringLiteral("https") || scheme == QStringLiteral("http")) &&
           !url.host().isEmpty() && url.userInfo().isEmpty() &&
           url.toString(QUrl::FullyEncoded).size() <= 2048;
  }

  void appendMessage(const QString& role, const QString& text, bool markdown) {
    QTextCursor cursor(document());
    cursor.movePosition(QTextCursor::End);
    if (!document()->isEmpty()) cursor.insertBlock();

    QTextCharFormat role_format;
    role_format.setFontWeight(QFont::DemiBold);
    cursor.insertText(role + QLatin1Char('\n'), role_format);
    if (markdown) {
      QTextDocument::MarkdownFeatures safe_features =
          QTextDocument::MarkdownDialectGitHub;
      safe_features |= QTextDocument::MarkdownNoHTML;
      cursor.insertFragment(QTextDocumentFragment::fromMarkdown(text, safe_features));
    } else {
      cursor.insertText(text);
    }
    setTextCursor(cursor);
    ensureCursorVisible();
  }

 protected:
  QVariant loadResource(int type, const QUrl& name) override {
    if (type == QTextDocument::ImageResource) return {};
    if (!name.scheme().isEmpty()) return {};
    return QTextBrowser::loadResource(type, name);
  }

 private:
  SafeLinkHandler safe_link_handler_;
};
