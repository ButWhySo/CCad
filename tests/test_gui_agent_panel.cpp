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
#include <QDir>
#include <QLabel>
#include <QTableWidget>
#include <QDialog>

#include "ccad_gui/agent_settings_dialog.hpp"
#include "ccad_gui/agent_marketplace_dialog.hpp"
#include "ccad_gui/agent_panel.hpp"

class TestGuiAgentPanel : public QObject {
  Q_OBJECT

 private:
  QTemporaryDir test_appdata_;

private slots:
  void initTestCase() {
    QVERIFY2(test_appdata_.isValid(), "temporary APPDATA directory must exist");
    qputenv("APPDATA", test_appdata_.path().toUtf8());
  }

  void cleanupTestCase() { qunsetenv("APPDATA"); }

  void testPanelReadsPersistedProviderModelBeforeBackendStartup() {
    const QString config_dir = QDir(test_appdata_.path()).filePath("CCad");
    QVERIFY(QDir().mkpath(config_dir));
    QFile config(QDir(config_dir).filePath("agent_config.json"));
    QVERIFY(config.open(QIODevice::WriteOnly | QIODevice::Truncate));
    config.write(QJsonDocument(QJsonObject{{"provider", "cerebras"},
                                           {"model", "qwen-3.8-27b"}})
                     .toJson(QJsonDocument::Compact));
    config.close();

    AgentPanel panel;
    const QJsonObject workspace = QJsonDocument::fromJson(panel.workspaceStateJson().toUtf8()).object();
    QCOMPARE(workspace.value("model_label").toString(), QString("Model: qwen-3.8-27b"));
    QCOMPARE(workspace.value("provider_id").toString(), QString("cerebras"));
    QVERIFY(!workspace.value("native_tool_catalog_installed").toBool());
    QCOMPARE(workspace.value("native_tool_catalog_method_count").toInt(), 0);
    QCOMPARE(workspace.value("context_schema_version").toInt(), 0);
    QCOMPARE(workspace.value("context_content_size").toInt(), 0);
    QCOMPARE(workspace.value("context_memory_entry_count").toInt(), 0);
    QVERIFY(workspace.value("context_sources").toArray().isEmpty());
  }

  void testSettingsDialogInteractions() {
    AgentPanel panel;
    AgentSettingsDialog dialog(&panel);

    QTimer::singleShot(0, &dialog, &QWidget::show);
    QTest::qWait(100);

    auto* save_btn = dialog.findChild<QPushButton*>("action:primaryButton");
    QVERIFY(save_btn != nullptr);
    auto* mcp_table = dialog.findChild<QTableWidget*>("control:mcpServersTable");
    QVERIFY(mcp_table != nullptr);
    QVERIFY(dialog.findChild<QLabel*>("label:mcpRuntimeStatus") != nullptr);
    auto* add_mcp = dialog.findChild<QPushButton*>("action:addMcpServerBtn");
    auto* remove_mcp = dialog.findChild<QPushButton*>("action:removeMcpServerBtn");
    QVERIFY(add_mcp != nullptr);
    QVERIFY(remove_mcp != nullptr);
    QTest::mouseClick(add_mcp, Qt::LeftButton);
    QCOMPARE(mcp_table->rowCount(), 1);
    QTest::mouseClick(remove_mcp, Qt::LeftButton);
    QCOMPARE(mcp_table->rowCount(), 0);

    // 1. General Tab
    auto* provider_combo = dialog.findChild<QComboBox*>("control:providerCombo");
    QVERIFY(provider_combo != nullptr);
    QVERIFY(provider_combo->findData("openai_compatible") >= 0);
    QVERIFY(provider_combo->findData("local_model") >= 0);
    QVERIFY(provider_combo->findData("ollama") >= 0);
    QTest::keyClick(provider_combo, Qt::Key_Down);

    auto* model_input = dialog.findChild<QLineEdit*>("control:modelInput");
    QVERIFY(model_input != nullptr);
    auto* model_combo = dialog.findChild<QComboBox*>("control:modelCombo");
    QVERIFY(model_combo != nullptr);
    QVERIFY(model_combo->isEditable());
    QVERIFY(model_combo->count() > 0);

    // Provider/model selection must carry one exact model ID at a time.
    const int cerebras_index = provider_combo->findData("cerebras");
    QVERIFY(cerebras_index >= 0);
    provider_combo->setCurrentIndex(cerebras_index);
    QCoreApplication::processEvents();
    QCOMPARE(model_combo->findText("gpt-oss-120b") >= 0, true);
    QCOMPARE(model_combo->findText("qwen-3.8-27b") >= 0, true);
    model_combo->setCurrentText("qwen-3.8-27b");
    QCOMPARE(model_combo->currentText(), QString("qwen-3.8-27b"));
    const int gemini_index = provider_combo->findData("google_gemini");
    QVERIFY(gemini_index >= 0);
    provider_combo->setCurrentIndex(gemini_index);
    QCoreApplication::processEvents();
    QVERIFY(!model_combo->currentText().contains("qwen-3.8-27b"));
    const int ollama_index = provider_combo->findData("ollama");
    QVERIFY(ollama_index >= 0);
    provider_combo->setCurrentIndex(ollama_index);
    QCoreApplication::processEvents();
    QVERIFY(model_combo->findText("qwen3") >= 0);
    QVERIFY(!model_input->isReadOnly());
    const int openrouter_index = provider_combo->findData("openrouter");
    QVERIFY(openrouter_index >= 0);
    provider_combo->setCurrentIndex(openrouter_index);
    QCoreApplication::processEvents();
    QVERIFY(model_combo->findText("openrouter/free") >= 0);

    QVERIFY(dialog.findChild<QLabel*>("label:modelDetails") != nullptr);
    auto* config_preview = dialog.findChild<QTextEdit*>("control:resolvedConfigPreview");
    QVERIFY(config_preview != nullptr);
    QVERIFY(config_preview->isReadOnly());
    QVERIFY(config_preview->toPlainText().contains("[agent]"));
    model_input->selectAll();
    QTest::keyClicks(model_input, "gpt-4o-test");

    auto* theme_combo = dialog.findChild<QComboBox*>("control:themeCombo");
    QVERIFY(theme_combo != nullptr);
    auto* grid_combo = dialog.findChild<QComboBox*>("control:gridCombo");
    QVERIFY(grid_combo != nullptr);
    QVERIFY(grid_combo->findText("0.5 mm") >= 0);
    QVERIFY(grid_combo->findText("5.0 mm") >= 0);
    QVERIFY(grid_combo->findText("Hidden") >= 0);
    QVERIFY(dialog.findChild<QCheckBox*>("control:autosaveCb") != nullptr);
    QVERIFY(dialog.findChild<QCheckBox*>("control:restoreSessionCb") != nullptr);

    auto* api_key_input = dialog.findChild<QLineEdit*>("control:apiKeyInput");
    QVERIFY(api_key_input != nullptr);
    QCOMPARE(api_key_input->echoMode(), QLineEdit::Password);
    auto* reveal_key = dialog.findChild<QCheckBox*>("control:showApiKeyCb");
    QVERIFY(reveal_key != nullptr);
    QVERIFY(!reveal_key->toolTip().isEmpty());
    // Do not click Show key here: production behavior intentionally opens the
    // Windows account-password prompt. Exercise that security boundary in the
    // live GUI harness, not in unattended Qt unit tests.
    QTest::keyClicks(api_key_input, "test-secret-not-persisted");
    api_key_input->clear();
    QVERIFY(api_key_input->text().isEmpty());
    auto* test_provider = dialog.findChild<QPushButton*>("action:testProviderBtn");
    QVERIFY(test_provider != nullptr);
    QCOMPARE(test_provider->text(), QStringLiteral("Validate local setup (no network)"));
    QVERIFY(!test_provider->toolTip().isEmpty());
    // The explicit live probe is deliberately not clicked by this unit test:
    // it sends a billable, real provider request.  The desktop live-provider
    // harness owns that end-to-end verification with an OS-vault credential.
    auto* test_connection = dialog.findChild<QPushButton*>("action:testProviderConnectionBtn");
    QVERIFY(test_connection != nullptr);
    QCOMPARE(test_connection->text(), QStringLiteral("Test live connection (uses quota)"));
    QVERIFY(test_connection->toolTip().contains("exactly one"));
    auto* set_provider_key = dialog.findChild<QPushButton*>("action:setProviderKeyBtn");
    QVERIFY(set_provider_key != nullptr);
    QCOMPARE(set_provider_key->text(), QStringLiteral("Set key"));
    QVERIFY(!set_provider_key->toolTip().isEmpty());
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
    QCOMPARE(clear_key->text(), QStringLiteral("Remove saved key"));
    QVERIFY(!clear_key->toolTip().isEmpty());

    auto* sandbox_cb = dialog.findChild<QCheckBox*>("control:sandboxCb");
    QVERIFY(sandbox_cb != nullptr);
    QTest::mouseClick(sandbox_cb, Qt::LeftButton);

    auto* approval_cb = dialog.findChild<QCheckBox*>("control:approvalCb");
    QVERIFY(approval_cb != nullptr);
    QTest::mouseClick(approval_cb, Qt::LeftButton);

    auto* project_name = dialog.findChild<QLineEdit*>("control:projectNameInput");
    QVERIFY(project_name != nullptr);
    project_name->selectAll();
    QTest::keyClicks(project_name, "test_proj");
    QCOMPARE(project_name->text(), QString("test_proj"));

    auto* project_path = dialog.findChild<QLineEdit*>("control:projectPathInput");
    QVERIFY(project_path != nullptr);
    project_path->selectAll();
    QTest::keyClicks(project_path, "/tmp/test");
    QCOMPARE(project_path->text(), QString("/tmp/test"));

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
    // Async config/catalog responses must not call the destroyed dialog.
    QTest::qWait(750);
  }

  void testPanelSettingsActionOpensOneRetainedDialog() {
    AgentPanel panel;
    panel.show();
    QTest::qWait(50);

    auto* settings = panel.findChild<QPushButton*>("action:settingsBtn");
    QVERIFY(settings != nullptr);
    QTest::mouseClick(settings, Qt::LeftButton);

    AgentSettingsDialog* dialog = nullptr;
    QTRY_VERIFY_WITH_TIMEOUT(([&]() {
      for (QWidget* widget : QApplication::topLevelWidgets()) {
        if (widget->objectName() == "dialog:agent_settings" && widget->isVisible()) {
          dialog = qobject_cast<AgentSettingsDialog*>(widget);
          return dialog != nullptr;
        }
      }
      return false;
    })(), 1000);

    QTest::mouseClick(settings, Qt::LeftButton);
    int visible_settings_count = 0;
    for (QWidget* widget : QApplication::topLevelWidgets()) {
      if (widget->objectName() == "dialog:agent_settings" && widget->isVisible()) {
        ++visible_settings_count;
      }
    }
    QCOMPARE(visible_settings_count, 1);
    dialog->close();
    QTest::qWait(50);
  }

  void testMarketplaceInteractions() {
    AgentPanel panel;
    AgentMarketplaceDialog dialog(&panel, &panel);

    QTimer::singleShot(0, &dialog, &QWidget::show);
    QTest::qWait(100);

    QVERIFY(dialog.isVisible());
    dialog.accept();
  }

  void testProviderSwitchDoesNotConcatenateModelPresets() {
    AgentPanel panel;
    AgentSettingsDialog dialog(&panel);

    auto* providers = dialog.findChild<QComboBox*>("control:providerCombo");
    auto* models = dialog.findChild<QComboBox*>("control:modelCombo");
    auto* model_input = dialog.findChild<QLineEdit*>("control:modelInput");
    QVERIFY(providers != nullptr);
    QVERIFY(models != nullptr);
    QVERIFY(model_input != nullptr);

    const int cerebras = providers->findData("cerebras");
    const int openrouter = providers->findData("openrouter");
    QVERIFY(cerebras >= 0);
    QVERIFY(openrouter >= 0);

    providers->setCurrentIndex(cerebras);
    QCoreApplication::processEvents();
    QCOMPARE(model_input->text(), QString("gpt-oss-120b"));
    QVERIFY(!model_input->text().contains("openrouter/"));

    // Simulate stale editable text from an older build, then change provider.
    model_input->setReadOnly(false);
    model_input->setText("openrouter/anthropic/claude-3.5-sonnetgpt-oss-120b");
    providers->setCurrentIndex(openrouter);
    QCoreApplication::processEvents();
    QVERIFY(!model_input->text().contains("gpt-oss-120b"));
    QVERIFY(!model_input->text().contains("claude-3.5-sonnetgpt-oss-120b"));
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

  void testModelPresetSwitch() {
    AgentPanel panel;
    AgentSettingsDialog dialog(&panel);
    auto* provider = dialog.findChild<QComboBox*>("control:providerCombo");
    auto* models = dialog.findChild<QComboBox*>("control:modelCombo");
    QVERIFY(provider != nullptr);
    QVERIFY(models != nullptr);
    provider->setCurrentIndex(provider->findData("cerebras"));
    QCOMPARE(models->currentText(), QString("gpt-oss-120b"));
    QVERIFY(models->findText("qwen-3.8-27b") >= 0);
    models->setEditText("claude-opus-5gpt-5.1-test");
    provider->setCurrentIndex(provider->findData("anthropic"));
    QCOMPARE(models->currentText(), QString("claude-opus-5"));
    provider->setCurrentIndex(provider->findData("openai"));
    QCOMPARE(models->currentText(), QString("gpt-5.1"));
  }

  void testProviderSecretSelectsStatusProvider() {
    AgentPanel panel;
    auto* provider_state = panel.findChild<QComboBox*>("control:providerStateSelector");
    QVERIFY(provider_state != nullptr);
    QVERIFY(provider_state->findData("cerebras") >= 0);

    panel.setProviderSecret("cerebras", "test-key-not-networked");
    QCOMPARE(provider_state->currentData().toString(), QString("cerebras"));
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
