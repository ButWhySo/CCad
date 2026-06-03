#include "ui_map_server.hpp"

#include "review_window.hpp"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QLocalSocket>

namespace {

QString jsonString(const QString& value) {
  QString output = "\"";
  for (const QChar ch : value) {
    if (ch == '\\') {
      output += "\\\\";
    } else if (ch == '"') {
      output += "\\\"";
    } else if (ch == '\n') {
      output += "\\n";
    } else if (ch == '\r') {
      output += "\\r";
    } else if (ch == '\t') {
      output += "\\t";
    } else {
      output += ch;
    }
  }
  output += "\"";
  return output;
}

QString errorResponse(const QString& reason) {
  return "{\"schema_version\":1,\"ok\":false,\"reason\":" + jsonString(reason) + "}";
}

QString oneLineJson(QString json) {
  json.remove('\n');
  json.remove('\r');
  return json;
}

}  // namespace

UiMapServer::UiMapServer(ReviewWindow& window) : window_(window) {
  QObject::connect(&server_, &QLocalServer::newConnection, &server_,
                   [this]() { acceptPendingConnections(); });
}

UiMapServer::~UiMapServer() {
  close();
}

bool UiMapServer::listen(const QString& server_name) {
  close();
  server_name_ = server_name;
  QLocalServer::removeServer(server_name_);
  return server_.listen(server_name_);
}

void UiMapServer::close() {
  if (!server_name_.isEmpty()) {
    server_.close();
    QLocalServer::removeServer(server_name_);
    server_name_.clear();
  }
}

QString UiMapServer::errorString() const {
  return server_.errorString();
}

QString UiMapServer::serverName() const {
  return server_name_;
}

void UiMapServer::acceptPendingConnections() {
  while (QLocalSocket* socket = server_.nextPendingConnection()) {
    QObject::connect(socket, &QLocalSocket::readyRead, socket,
                     [this, socket]() { handleReadyRead(*socket); });
    QObject::connect(socket, &QLocalSocket::disconnected, socket, &QLocalSocket::deleteLater);
  }
}

void UiMapServer::handleReadyRead(QLocalSocket& socket) {
  while (socket.canReadLine()) {
    const QString line = QString::fromUtf8(socket.readLine()).trimmed();
    if (line.isEmpty()) {
      continue;
    }
    const QByteArray response = (responseForLine(line) + "\n").toUtf8();
    socket.write(response);
    socket.flush();
  }
}

QString UiMapServer::responseForLine(const QString& line) const {
  QJsonParseError error;
  const QJsonDocument document = QJsonDocument::fromJson(line.toUtf8(), &error);
  if (error.error != QJsonParseError::NoError || !document.isObject()) {
    return errorResponse("invalid json request");
  }
  const QJsonObject request = document.object();
  const QString method = request.value("method").toString();
  if (method.isEmpty()) {
    return errorResponse("request requires string method");
  }
  return oneLineJson(window_.runAgentUiQueryJson(method, line));
}
