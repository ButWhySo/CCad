#include "ui_map_server.hpp"

#include "review_window.hpp"

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

QString extractJsonString(const QString& json, const QString& key) {
  const QString token = "\"" + key + "\":";
  const int token_index = json.indexOf(token);
  if (token_index < 0) {
    return {};
  }
  int index = token_index + token.size();
  while (index < json.size() && json.at(index).isSpace()) {
    ++index;
  }
  if (index >= json.size() || json.at(index) != '"') {
    return {};
  }
  ++index;
  QString value;
  while (index < json.size()) {
    const QChar ch = json.at(index++);
    if (ch == '"') {
      return value;
    }
    if (ch == '\\' && index < json.size()) {
      const QChar escaped = json.at(index++);
      if (escaped == 'n') {
        value += '\n';
      } else if (escaped == 'r') {
        value += '\r';
      } else if (escaped == 't') {
        value += '\t';
      } else {
        value += escaped;
      }
    } else {
      value += ch;
    }
  }
  return {};
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
  const QString method = extractJsonString(line, "method");
  if (method == "ui.map") {
    return "{\"schema_version\":1,\"ok\":true,\"method\":\"ui.map\",\"result\":" +
           oneLineJson(window_.uiMapJson()) + "}";
  }
  if (method == "ui.target") {
    const QString id = extractJsonString(line, "id");
    if (id.isEmpty()) {
      return errorResponse("ui.target requires string id");
    }
    return "{\"schema_version\":1,\"ok\":true,\"method\":\"ui.target\",\"result\":" +
           oneLineJson(window_.uiTargetJsonById(id)) + "}";
  }
  if (method == "ui.active_layer") {
    return "{\"schema_version\":1,\"ok\":true,\"method\":\"ui.active_layer\",\"result\":" +
           oneLineJson(window_.activePcbLayerJson()) + "}";
  }
  if (method == "ui.set_active_layer") {
    const QString layer_id = extractJsonString(line, "layer_id");
    if (layer_id.isEmpty()) {
      return errorResponse("ui.set_active_layer requires string layer_id");
    }
    return "{\"schema_version\":1,\"ok\":true,\"method\":\"ui.set_active_layer\",\"result\":" +
           oneLineJson(window_.setActivePcbLayerForAutomation(layer_id)) + "}";
  }
  if (method == "ui.epoch") {
    const QString map = window_.uiMapJson();
    const int key_index = map.indexOf("\"ui_epoch\":");
    QString epoch = "0";
    if (key_index >= 0) {
      int index = key_index + 11;
      while (index < map.size() && map.at(index).isSpace()) {
        ++index;
      }
      int end = index;
      while (end < map.size() && map.at(end).isDigit()) {
        ++end;
      }
      if (end > index) {
        epoch = map.mid(index, end - index);
      }
    }
    return "{\"schema_version\":1,\"ok\":true,\"method\":\"ui.epoch\",\"ui_epoch\":" + epoch +
           "}";
  }
  return errorResponse("unsupported method");
}
