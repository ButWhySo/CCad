#pragma once

#include <QJsonObject>
#include <QString>
#include <QUuid>

namespace ccad::gui {

inline QJsonObject makeAgentApprovalMetadata(const QString& proposal_id,
                                             const QString& approval_id,
                                             const QString& decision) {
  const auto valid_uuid = [](const QString& value) {
    const QUuid parsed = QUuid::fromString(value);
    return !parsed.isNull() &&
           parsed.toString(QUuid::WithoutBraces).compare(value, Qt::CaseInsensitive) == 0;
  };
  if (!valid_uuid(proposal_id) || !valid_uuid(approval_id) ||
      (decision != QStringLiteral("approved") &&
       decision != QStringLiteral("rejected") &&
       decision != QStringLiteral("cancelled"))) {
    return {};
  }
  return {{"schema_version", 1}, {"proposal_id", proposal_id.toLower()},
          {"approval_id", approval_id.toLower()},
          {"approval_decision", decision}};
}

}  // namespace ccad::gui
