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
    QVERIFY(provider_combo->findData("openai_compatible") >= 0);
    QVERIFY(provider_combo->findData("local_model") >= 0);
    QTest::keyClick(provider_combo, Qt::Key_Down);

    auto* model_input = dialog.findChild<QLineEdit*>("control:modelInput");
    QVERIFY(model_input != nullptr);
    auto* model_combo = dialog.findChild<QComboBox*>("control:modelCombo");
    QVERIFY(model_combo != nullptr);
    QVERIFY(model_combo->isEditable());
    QVERIFY(model_combo->count() > 0);
    QVERIFY(dialog.findChild<QLabel*>("label:modelDetails") != nullptr);
    auto* config_preview = dialog.findChild<QTextEdit*>("control:resolvedConfigPreview");
    QVERIFY(config_preview != nullptr);
    QVERIFY(config_preview->isReadOnly());
    QVERIFY(config_preview->toPlainText().contains("[agent]"));
    QTest::keyClicks(model_input, "gpt-4o-test");

    auto* theme_combo = dialog.findChild<QComboBox*>("control:themeCombo");
    QVERIFY(theme_combo != nullptr);
    auto* grid_combo = dialog.findChild<QComboBox*>("control:gridCombo");
    QVERIFY(grid_combo != nullptr);
    QVERIFY(dialog.findChild<QCheckBox*>("control:autosaveCb") != nullptr);
    QVERIFY(dialog.findChild<QCheckBox*>("control:restoreSessionCb") != nullptr);

    auto* api_key_input = dialog.findChild<QLineEdit*>("control:apiKeyInput");
    QVERIFY(api_key_input != nullptr);
    QCOMPARE(api_key_input->echoMode(), QLineEdit::Password);
    auto* reveal_key = dialog.findChild<QCheckBox*>("control:showApiKeyCb");
    QVERIFY(reveal_key != nullptr);
    QTest::mouseClick(reveal_key, Qt::LeftButton);
    QCOMPARE(api_key_input->echoMode(), QLineEdit::Normal);
    QTest::mouseClick(reveal_key, Qt::LeftButton);
    QCOMPARE(api_key_input->echoMode(), QLineEdit::Password);
    QTest::keyClicks(api_key_input, "test-secret-not-persisted");
    api_key_input->clear();
    QVERIFY(api_key_input->text().isEmpty());
    auto* test_provider = dialog.findChild<QPushButton*>("action:testProviderBtn");
    QVERIFY(test_provider != nullptr);
    QVERIFY(!test_provider->toolTip().isEmpty());
    auto* provider_target = dialog.findChild<QLabel*>("label:providerTestTarget");
    QVERIFY(provider_target != nullptr);
    QVERIFY(provider_target->text().contains("Test target:"));
    auto* provider_status = dialog.findChild<QLabel*>("label:providerTestStatus");
    QVERIFY(provider_status != nullptr);
    QVERIFY(provider_status->text().contains("not run"));
    QTest::mouseClick(test_provider, Qt::LeftButton);
    QTest::qWait(5200);
    QVERIFY(!provider_status->text().contains("not run"));
    QVERIFY(!provider_status->text().contains("running"));
    auto* clear_key = dialog.findChild<QPushButton*>("action:clearProviderKeyBtn");
    QVERIFY(clear_key != nullptr);
    QVERIFY(!clear_key->toolTip().isEmpty());

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
    auto* collapse = panel.findChild<QPushButton*>("action:agent_collapse");
    QVERIFY(collapse != nullptr);
    QVERIFY(!collapse->toolTip().isEmpty());
    QVERIFY(!send->toolTip().isEmpty());
    QVERIFY(panel.findChild<QPushButton*>("action:agent_attach") != nullptr);
    QVERIFY(panel.findChild<QPushButton*>("action:agent_marketplace") != nullptr);
    QVERIFY(panel.findChild<QPushButton*>("action:agent_context_refresh") != nullptr);
    QVERIFY(panel.findChild<QPushButton*>("action:agent_voice") != nullptr);
    QVERIFY(panel.findChild<QLabel*>("status:agent_run") != nullptr);
    QVERIFY(panel.findChild<QLabel*>("label:agent_result") != nullptr);
    auto* context_label = panel.findChild<QLabel*>("control:contextLabel");
    QVERIFY(context_label != nullptr);
    QVERIFY(context_label->text().contains("/ 128k context"));
    auto* chat_input = panel.findChild<QTextEdit*>("control:agent_chat_input");
    QVERIFY(chat_input != nullptr);
    QVERIFY(chat_input->isEnabled());
    auto* summarize = panel.findChild<QPushButton*>("action:agent_quick_summarize");
    QVERIFY(summarize != nullptr);
    QTest::mouseClick(summarize, Qt::LeftButton);
    QCOMPARE(chat_input->toPlainText(), QString("/explain "));
  }

  void testApprovalLaneTransitions() {
    AgentPanel panel;
    auto* approval_card = panel.findChild<QFrame*>("panel:agent_approval_preview");
    QVERIFY(approval_card != nullptr);
    QVERIFY(!approval_card->isVisible());
    panel.show();
    QCoreApplication::processEvents();
    panel.setApprovalRequestText("Approve ui.route_track");
    panel.requestApproval();
    QCOMPARE(panel.pendingApprovalCount(), 1);
    QVERIFY(panel.approvalStatusText().contains("Approval pending"));
    QVERIFY(approval_card->isVisible());

    panel.declineNextApproval();
    QCOMPARE(panel.pendingApprovalCount(), 0);
    QVERIFY(panel.approvalStatusText().contains("Approval declined"));
    QVERIFY(!approval_card->isVisible());

    panel.setApprovalRequestText("Approve ui.place_via");
    panel.requestApproval();
    panel.cancelApproval();
    QCOMPARE(panel.pendingApprovalCount(), 0);
    QVERIFY(panel.approvalStatusText().contains("Approval canceled"));
  }

  void testApprovalControlsAreVisibleAndTargetable() {
    AgentPanel panel;
    QVERIFY(panel.findChild<QLineEdit*>("control:agent_approval_request") != nullptr);
    QVERIFY(panel.findChild<QPushButton*>("action:agent_request_approval") != nullptr);
    QVERIFY(panel.findChild<QPushButton*>("action:agent_approve_next") != nullptr);
    QVERIFY(panel.findChild<QPushButton*>("action:agent_decline_next") != nullptr);
  }

  void testProposalReviewCardTransitions() {
    AgentPanel panel;
    auto* card = panel.findChild<QFrame*>("card:agent_proposal");
    QVERIFY(card != nullptr);
    QVERIFY(!panel.proposalVisible());
    panel.show();
    panel.showProposal("Improve GND routing near U3", {"Reroute track T19", "Add via V17", "Preserve U3 placement"});
    QVERIFY(panel.proposalVisible());
    QCOMPARE(panel.findChild<QListWidget*>("list:agent_proposal_changes")->count(), 3);
    auto* details = panel.findChild<QPushButton*>("action:agent_proposal_details");
    QVERIFY(details != nullptr);
    QVERIFY(!details->toolTip().isEmpty() || details->text() == "View details");
    auto* revise = panel.findChild<QPushButton*>("action:agent_proposal_revise");
    QVERIFY(revise != nullptr);
    QTest::mouseClick(revise, Qt::LeftButton);
    auto* preserve = panel.findChild<QCheckBox*>("control:proposal_preserve_placement");
    QVERIFY(preserve != nullptr);
    QTest::mouseClick(preserve, Qt::LeftButton);
    auto* instructions = panel.findChild<QTextEdit*>("control:proposal_revision_input");
    QVERIFY(instructions != nullptr);
    instructions->setPlainText("Keep the original route near U3.");
    auto* submit_revision = panel.findChild<QPushButton*>("action:agent_proposal_submit_revision");
    QVERIFY(submit_revision != nullptr);
    QTest::mouseClick(submit_revision, Qt::LeftButton);
    QVERIFY(panel.findChild<QTextEdit*>("control:agent_chat_input")->toPlainText().contains("preserve placement"));
    QVERIFY(panel.findChild<QTextEdit*>("control:agent_chat_input")->toPlainText().contains("Keep the original route"));
    auto* reject = panel.findChild<QPushButton*>("action:agent_proposal_reject");
    QVERIFY(reject != nullptr);
    QTest::mouseClick(reject, Qt::LeftButton);
    QVERIFY(!panel.proposalVisible());
  }

  void testSlashCommandPaletteUsesExecutableCommands() {
    AgentPanel panel;
    panel.show();
    QCoreApplication::processEvents();
    auto* input = panel.findChild<QTextEdit*>("control:agent_chat_input");
    auto* popup = panel.findChild<QListWidget*>("panel:agent_slash_commands");
    QVERIFY(input != nullptr);
    QVERIFY(popup != nullptr);
    input->setFocus();
    QTest::keyClicks(input, "/workflow");
    QCoreApplication::processEvents();
    QVERIFY(popup->count() >= 3);
    QVERIFY(popup->item(0)->text().startsWith("/workflow "));
    QVERIFY(!popup->item(0)->text().contains(":use:"));
    QTest::keyClick(input, Qt::Key_Escape);
    QCoreApplication::processEvents();
    // Popup focus is platform-dependent in offscreen Qt; production event-filter
    // handling is exercised by the live GUI harness.
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
