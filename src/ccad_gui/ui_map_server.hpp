#pragma once

#include <QLocalServer>
#include <QString>

class ReviewWindow;
class QLocalSocket;

class UiMapServer {
 public:
  explicit UiMapServer(ReviewWindow& window);
  ~UiMapServer();

  bool listen(const QString& server_name);
  void close();
  QString errorString() const;
  QString serverName() const;

 private:
  void acceptPendingConnections();
  void handleReadyRead(QLocalSocket& socket);
  QString responseForLine(const QString& line) const;

  ReviewWindow& window_;
  QLocalServer server_;
  QString server_name_;
};
