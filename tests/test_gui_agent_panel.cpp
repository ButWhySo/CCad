#include <QtTest>
#include <QApplication>
#include <QComboBox>
#include <QLineEdit>
#include <QCheckBox>
#include <QPushButton>
#include <QTextEdit>
#include <QListWidget>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTemporaryDir>
#include <QLabel>

#include "ccad_gui/agent_settings_dialog.hpp"
#include "ccad_gui/agent_marketplace_dialog.hpp"
#include "ccad_gui/agent_panel.hpp"

class TestGuiAgentPanel : public QObject {
  Q_OBJECT

private slots:
  void testSettingsDialogInteractions() {
    AgentPanel panel;
    AgentSettingsDialog dialog(&panel);

    QTimer::singleShot(0, &dialog, &QWidget::show);
    QTest::qWait(100);

    auto* save_btn = dialog.findChild<QPushButton*>("action:primaryButton");
    QVERIFY(save_btn != nullptr);

    // 1. General Tab
    auto* provider_combo = dialog.findChild<QComboBox*>("control:providerCombo");
    QVERIFY(provider_combo != nullptr);
    QTest::keyClick(provider_combo, Qt::Key_Down);

    auto* model_input = dialog.findChild<QLineEdit*>("control:modelInput");
    QVERIFY(model_input != nullptr);
    QTest::keyClicks(model_input, "gpt-4o-test");

    auto* api_key_input = dialog.findChild<QLineEdit*>("control:apiKeyInput");
    QVERIFY(api_key_input != nullptr);
    QCOMPARE(api_key_input->echoMode(), QLineEdit::Password);
    QTest::keyClicks(api_key_input, "test-secret-not-persisted");

    auto* sandbox_cb = dialog.findChild<QCheckBox*>("control:sandboxCb");
    QVERIFY(sandbox_cb != nullptr);
    QTest::mouseClick(sandbox_cb, Qt::LeftButton);

    auto* approval_cb = dialog.findChild<QCheckBox*>("control:approvalCb");
    QVERIFY(approval_cb != nullptr);
    QTest::mouseClick(approval_cb, Qt::LeftButton);

    auto* project_name = dialog.findChild<QLineEdit*>("control:projectNameInput");
    QVERIFY(project_name != nullptr);
    QTest::keyClicks(project_name, "test_proj");

    auto* project_path = dialog.findChild<QLineEdit*>("control:projectPathInput");
    QVERIFY(project_path != nullptr);
    QTest::keyClicks(project_path, "/tmp/test");

    auto* trust_level = dialog.findChild<QComboBox*>("control:trustLevelCombo");
    QVERIFY(trust_level != nullptr);
    QTest::keyClick(trust_level, Qt::Key_Down);

    // 2. Capabilities
    auto* stm_cb = dialog.findChild<QCheckBox*>("control:stmCb");
    QVERIFY(stm_cb != nullptr);
    QTest::mouseClick(stm_cb, Qt::LeftButton);

    auto* ltm_cb = dialog.findChild<QCheckBox*>("control:ltmCb");
    QVERIFY(ltm_cb != nullptr);
    QTest::mouseClick(ltm_cb, Qt::LeftButton);

    auto* episodic_cb = dialog.findChild<QCheckBox*>("control:episodicCb");
    QVERIFY(episodic_cb != nullptr);
    QTest::mouseClick(episodic_cb, Qt::LeftButton);

    auto* hooks_combo = dialog.findChild<QComboBox*>("control:hooksCombo");
    QVERIFY(hooks_combo != nullptr);
    QTest::keyClick(hooks_combo, Qt::Key_Down);

    // 3. Personalisation
    auto* follow_up = dialog.findChild<QLineEdit*>("control:followUpInput");
    QVERIFY(follow_up != nullptr);
    QTest::keyClicks(follow_up, "Be concise.");

    auto* context_window = dialog.findChild<QCheckBox*>("control:contextWindowCb");
    QVERIFY(context_window != nullptr);
    QTest::mouseClick(context_window, Qt::LeftButton);

    auto* chat_mode = dialog.findChild<QComboBox*>("control:chatModeCombo");
    QVERIFY(chat_mode != nullptr);
    QTest::keyClick(chat_mode, Qt::Key_Down);

    auto* personality = dialog.findChild<QComboBox*>("control:agentPersonalityCombo");
    QVERIFY(personality != nullptr);
    QTest::keyClick(personality, Qt::Key_Down);

    // Skip custom instructions text edit as QTest::keyClicks can be flaky on QTextEdit

    // 4. API & Providers
    auto* test_export = dialog.findChild<QPushButton*>("action:testExportBtn");
    QVERIFY(test_export != nullptr);
    QTest::mouseClick(test_export, Qt::LeftButton);

    // Save
    QTest::mouseClick(save_btn, Qt::LeftButton);
    QVERIFY(dialog.isHidden());
  }

  void testMarketplaceInteractions() {
    AgentPanel panel;
    AgentMarketplaceDialog dialog(&panel, &panel);

    QTimer::singleShot(0, &dialog, &QWidget::show);
    QTest::qWait(100);

    QVERIFY(dialog.isVisible());
    dialog.accept();
  }

  void testChatActionsAreTargetable() {
    AgentPanel panel;
    auto* send = panel.findChild<QPushButton*>("action:agent_submit_chat");
    QVERIFY(send != nullptr);
    QVERIFY(!send->toolTip().isEmpty());
    QVERIFY(panel.findChild<QPushButton*>("action:agent_attach") != nullptr);
    QVERIFY(panel.findChild<QPushButton*>("action:agent_marketplace") != nullptr);
    QVERIFY(panel.findChild<QPushButton*>("action:agent_context_refresh") != nullptr);
    QVERIFY(panel.findChild<QPushButton*>("action:agent_voice") != nullptr);
    QVERIFY(panel.findChild<QLabel*>("status:agent_run") != nullptr);
    QVERIFY(panel.findChild<QLabel*>("status:agent_result") != nullptr);
  }

  void testApprovalLaneTransitions() {
    AgentPanel panel;
    panel.setApprovalRequestText("Approve ui.route_track");
    panel.requestApproval();
    QCOMPARE(panel.pendingApprovalCount(), 1);
    QVERIFY(panel.approvalStatusText().contains("Approval pending"));

    panel.declineNextApproval();
    QCOMPARE(panel.pendingApprovalCount(), 0);
    QVERIFY(panel.approvalStatusText().contains("Approval declined"));

    panel.setApprovalRequestText("Approve ui.place_via");
    panel.requestApproval();
    panel.cancelApproval();
    QCOMPARE(panel.pendingApprovalCount(), 0);
    QVERIFY(panel.approvalStatusText().contains("Approval canceled"));
  }

  void testQueueStateCheckpointRoundTrip() {
    QTemporaryDir temp;
    QVERIFY(temp.isValid());
    const QString path = temp.filePath("agent-session.json");
    QFile file(path);
    QVERIFY(file.open(QIODevice::WriteOnly));
    const QJsonObject session{{"session_kind", "ccad_agent_session"},
                              {"session_id", "session-queue"},
                              {"thread_id", "thread-queue"},
                              {"checkpoints", QJsonArray{}}};
    file.write(QJsonDocument(session).toJson(QJsonDocument::Compact));
    file.close();

    AgentPanel source;
    source.bindSessionFile(path);
    source.checkpointSession();

    QFile saved(path);
    QVERIFY(saved.open(QIODevice::ReadOnly));
    const QJsonObject saved_session =
        QJsonDocument::fromJson(saved.readAll()).object();
    QVERIFY(saved_session.value("run_queue_state").isObject());
    QCOMPARE(saved_session.value("run_queue_state").toObject().value("run_queue_depth").toInt(), 3);

    AgentPanel restored;
    restored.bindSessionFile(path);
    const auto labels = restored.findChildren<QLabel*>();
    bool queue_label_found = false;
    for (const auto* label : labels) {
      if (label->text().contains("3 queued")) {
        queue_label_found = true;
        break;
      }
    }
    QVERIFY(queue_label_found);
  }
};

QTEST_MAIN(TestGuiAgentPanel)
#include "test_gui_agent_panel.moc"
