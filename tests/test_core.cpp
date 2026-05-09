#include <QCoreApplication>
#include <QtTest/QtTest>
#include <QTimer>
#include <QUuid>
#include <QTcpServer>
#include <QTcpSocket>
#include <QHostAddress>
#include <QTemporaryDir>
#include <QFile>
#include <QDir>
#include <QTextStream>
#include <QStringConverter>

#include "memory/conversation_history.h"
#include "memory/memory_manager.h"
#include "permission/permission_manager.h"
#include "agent/react_agent_core.h"
#include "tool/tool_executor.h"
#include "tool/tool_registry.h"
#include "tool/i_tool.h"
#include "infrastructure/database/database_manager.h"
#include "infrastructure/database/conversation_store.h"
#include "infrastructure/network/http_client.h"
#include "application/template_loader.h"

namespace clawpp {

class MockProvider : public ILLMProvider {
public:
    explicit MockProvider(QObject* parent = nullptr)
        : ILLMProvider(parent)
        , abortCalled(false)
        , streamCalls(0) {}

    QString name() const override {
        return QStringLiteral("mock");
    }

    bool isAvailable() const override {
        return true;
    }

    QString lastError() const override {
        return QString();
    }

    void chat(const MessageList&,
              const ChatOptions&,
              std::function<void(const QString&)> onSuccess,
              std::function<void(const QString&)> onError) override {
        if (onError) {
            onError(QStringLiteral("chat not implemented in mock"));
        } else if (onSuccess) {
            onSuccess(QString());
        }
    }

    void chatStream(const MessageList&,
                    const ChatOptions&,
                    StreamCallback onToken,
                    std::function<void()> onComplete,
                    ErrorCallback onError) override {
        ++streamCalls;
        tokenCallback = std::move(onToken);
        completeCallback = std::move(onComplete);
        errorCallback = std::move(onError);
    }

    void abort() override {
        abortCalled = true;
    }

    void emitChunk(const QString& content) {
        if (!tokenCallback) {
            return;
        }
        StreamChunk chunk;
        chunk.content = content;
        tokenCallback(chunk);
    }

    void emitChunk(const StreamChunk& chunk) {
        if (!tokenCallback) {
            return;
        }
        tokenCallback(chunk);
    }

    void emitComplete() {
        if (completeCallback) {
            completeCallback();
        }
    }

    void emitError(const QString& error) {
        if (errorCallback) {
            errorCallback(error);
        }
    }

    bool abortCalled;
    int streamCalls;

private:
    StreamCallback tokenCallback;
    std::function<void()> completeCallback;
    ErrorCallback errorCallback;
};

class DummyTool : public ITool {
public:
    explicit DummyTool(const QString& toolName, QObject* parent = nullptr)
        : ITool(parent)
        , m_name(toolName) {}

    QString name() const override {
        return m_name;
    }

    QString description() const override {
        return QStringLiteral("Dummy tool for tests");
    }

    QJsonObject parameters() const override {
        QJsonObject schema;
        schema[QStringLiteral("type")] = QStringLiteral("object");
        return schema;
    }

    PermissionLevel permissionLevel() const override {
        return PermissionLevel::Moderate;
    }

    ToolResult execute(const QJsonObject&) override {
        ToolResult result;
        result.success = true;
        result.content = QStringLiteral("dummy-ok");
        return result;
    }

private:
    QString m_name;
};

class ConversationHistoryTests : public QObject {
    Q_OBJECT

private slots:
    void appendIncreasesCountAndTokens();
    void compressKeepsEdgesAndAddsSummary();
    void compressionRatioAffectsSummaryLength();
};

void ConversationHistoryTests::appendIncreasesCountAndTokens() {
    ConversationHistory history;
    QVERIFY(history.count() == 0);

    history.append(Message(MessageRole::User, QStringLiteral("hello world")));

    QCOMPARE(history.count(), 1);
    QVERIFY(history.totalTokens() > 0);
}

void ConversationHistoryTests::compressKeepsEdgesAndAddsSummary() {
    ConversationHistory history;
    MessageList messages;

    messages.append(Message(MessageRole::User, QStringLiteral("first")));
    messages.append(Message(MessageRole::Assistant, QStringLiteral("middle-1")));
    messages.append(Message(MessageRole::Assistant, QStringLiteral("middle-2")));
    messages.append(Message(MessageRole::User, QStringLiteral("last")));

    history.setMessages(messages);
    history.compressMiddle(1, 1, 0.5);

    const MessageList compressed = history.getAll();
    QCOMPARE(compressed.size(), 3);
    QCOMPARE(compressed.first().content, QStringLiteral("first"));
    QCOMPARE(compressed.last().content, QStringLiteral("last"));
    QVERIFY(compressed.at(1).role == MessageRole::System);
    QVERIFY(compressed.at(1).content.contains(QStringLiteral("summary"), Qt::CaseInsensitive));
}

void ConversationHistoryTests::compressionRatioAffectsSummaryLength() {
    MessageList source;
    source.append(Message(MessageRole::User, QStringLiteral("edge-first")));
    for (int i = 0; i < 12; ++i) {
        source.append(Message(
            MessageRole::Assistant,
            QStringLiteral("middle message %1 with long content long content long content long content")
                .arg(i)));
    }
    source.append(Message(MessageRole::User, QStringLiteral("edge-last")));

    ConversationHistory lowRatio;
    ConversationHistory highRatio;
    lowRatio.setMessages(source);
    highRatio.setMessages(source);

    lowRatio.compressMiddle(1, 1, 0.1);
    highRatio.compressMiddle(1, 1, 1.0);

    const QString lowSummary = lowRatio.getAll().at(1).content;
    const QString highSummary = highRatio.getAll().at(1).content;

    QVERIFY(highSummary.size() >= lowSummary.size());
}

class MemoryManagerTests : public QObject {
    Q_OBJECT

private slots:
    void rememberMessageCreatesAutoMemoryNote();
    void duplicateRememberMessageIsDeduped();
};

void MemoryManagerTests::rememberMessageCreatesAutoMemoryNote() {
    QTemporaryDir workspace;
    QVERIFY(workspace.isValid());

    MemoryManager manager(workspace.path());
    manager.setSessionContext(QStringLiteral("s-auto-memory"), QStringLiteral("Auto Memory Session"));
    manager.addMessage(Message(MessageRole::User, QStringLiteral("请记住：以后优先使用 CMake Presets。")));

    const QString memory = manager.readLongTerm();
    QVERIFY(memory.contains(QStringLiteral("## Auto Memory Notes")));
    QVERIFY(memory.contains(QStringLiteral("以后优先使用 CMake Presets")));
}

void MemoryManagerTests::duplicateRememberMessageIsDeduped() {
    QTemporaryDir workspace;
    QVERIFY(workspace.isValid());

    MemoryManager manager(workspace.path());
    manager.setSessionContext(QStringLiteral("s-dedupe"), QStringLiteral("Dedupe Session"));
    const QString note = QStringLiteral("记住：默认使用 cmake --build build -j 8");
    manager.addMessage(Message(MessageRole::User, note));
    manager.addMessage(Message(MessageRole::User, note));

    const QString memory = manager.readLongTerm();
    const QString normalized = QStringLiteral("默认使用 cmake --build build -j 8");
    const int occurrences = memory.count(normalized);
    QCOMPARE(occurrences, 1);
}

class TemplateLoaderTests : public QObject {
    Q_OBJECT

private slots:
    void memoryLoadUsesHeadBudget();
};

void TemplateLoaderTests::memoryLoadUsesHeadBudget() {
    QTemporaryDir workspace;
    QVERIFY(workspace.isValid());
    const QString memoryDir = QDir(workspace.path()).filePath(QStringLiteral("memory"));
    QVERIFY(QDir().mkpath(memoryDir));

    const QString memoryPath = QDir(memoryDir).filePath(QStringLiteral("MEMORY.md"));
    QFile file(memoryPath);
    QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
    QTextStream out(&file);
    out.setEncoding(QStringConverter::Utf8);
    for (int i = 0; i < 260; ++i) {
        out << "line-" << i << ": remember this long memory entry for truncation check\n";
    }
    file.close();

    TemplateLoader loader(workspace.path());
    const QString loaded = loader.loadMemory();

    QVERIFY(loaded.contains(QStringLiteral("line-0")));
    QVERIFY(!loaded.contains(QStringLiteral("line-250")));
    QVERIFY(loaded.contains(QStringLiteral("truncated"), Qt::CaseInsensitive));
}

class PermissionManagerTests : public QObject {
    Q_OBJECT

private slots:
    void defaultActionApplied();
    void wildcardRuleMatchesPath();
    void sessionWhitelistScoped();
    void cacheInvalidatedAfterRuleChange();
    void cacheInvalidatedAfterWhitelistChange();
};

void PermissionManagerTests::defaultActionApplied() {
    PermissionConfig config;
    config.defaultAction = PermissionRule::Action::Deny;
    PermissionManager::instance().initialize(config);

    ToolCall call;
    call.name = QStringLiteral("shell");
    call.path = QStringLiteral("C:/workspace/a.txt");

    QCOMPARE(PermissionManager::instance().checkPermission(QStringLiteral("s1"), call), PermissionResult::Denied);
}

void PermissionManagerTests::wildcardRuleMatchesPath() {
    PermissionConfig config;
    config.defaultAction = PermissionRule::Action::Deny;
    PermissionManager::instance().initialize(config);

    PermissionManager::instance().addRule(
        QStringLiteral("shell:C:/workspace/*"),
        PermissionRule::Action::Allow,
        QStringList{QStringLiteral("shell")});

    ToolCall call;
    call.name = QStringLiteral("shell");
    call.path = QStringLiteral("C:/workspace/project/run.ps1");

    QCOMPARE(PermissionManager::instance().checkPermission(QStringLiteral("s2"), call), PermissionResult::Allowed);
}

void PermissionManagerTests::sessionWhitelistScoped() {
    PermissionConfig config;
    config.defaultAction = PermissionRule::Action::Deny;
    PermissionManager::instance().initialize(config);

    PermissionManager::instance().addToWhitelist(
        QStringLiteral("session-alpha"),
        QStringLiteral("shell:C:/allowed/*"));

    ToolCall call;
    call.name = QStringLiteral("shell");
    call.path = QStringLiteral("C:/allowed/task.cmd");

    QCOMPARE(PermissionManager::instance().checkPermission(QStringLiteral("session-alpha"), call), PermissionResult::Allowed);
    QCOMPARE(PermissionManager::instance().checkPermission(QStringLiteral("session-beta"), call), PermissionResult::Denied);
}

void PermissionManagerTests::cacheInvalidatedAfterRuleChange() {
    PermissionConfig config;
    config.defaultAction = PermissionRule::Action::Deny;
    PermissionManager::instance().initialize(config);

    ToolCall call;
    call.name = QStringLiteral("shell");
    call.path = QStringLiteral("C:/workspace/cache-rule.txt");

    QCOMPARE(PermissionManager::instance().checkPermission(QStringLiteral("cache-rule"), call), PermissionResult::Denied);

    PermissionManager::instance().addRule(
        QStringLiteral("shell:C:/workspace/cache-rule.txt"),
        PermissionRule::Action::Allow,
        QStringList{QStringLiteral("shell")});

    QCOMPARE(PermissionManager::instance().checkPermission(QStringLiteral("cache-rule"), call), PermissionResult::Allowed);
}

void PermissionManagerTests::cacheInvalidatedAfterWhitelistChange() {
    PermissionConfig config;
    config.defaultAction = PermissionRule::Action::Deny;
    PermissionManager::instance().initialize(config);

    ToolCall call;
    call.name = QStringLiteral("shell");
    call.path = QStringLiteral("C:/allowed/cache-whitelist.cmd");

    QCOMPARE(PermissionManager::instance().checkPermission(QStringLiteral("cache-whitelist"), call), PermissionResult::Denied);

    PermissionManager::instance().addToWhitelist(
        QStringLiteral("cache-whitelist"),
        QStringLiteral("shell:C:/allowed/*"));

    QCOMPARE(PermissionManager::instance().checkPermission(QStringLiteral("cache-whitelist"), call), PermissionResult::Allowed);
}

class ReactAgentCoreTests : public QObject {
    Q_OBJECT

private slots:
    void streamCompleteProducesFinalResponse();
    void stopSuppressesLateCallbacks();
    void emptyStreamErrorUsesFallbackMessage();
    void noToolFirstIterationContinuesLoop();
    void invalidToolArgumentsBecomeToolMessage();
};

void ReactAgentCoreTests::streamCompleteProducesFinalResponse() {
    ReactAgentCore core;
    MockProvider provider;
    core.setProvider(&provider);

    QSignalSpy completedSpy(&core, &ReactAgentCore::completed);
    QSignalSpy errorSpy(&core, &ReactAgentCore::errorOccurred);

    core.run(QStringLiteral("hello"));
    QCOMPARE(provider.streamCalls, 1);

    provider.emitChunk(QStringLiteral("abc"));
    provider.emitComplete();

    QCOMPARE(errorSpy.count(), 0);
    QCOMPARE(completedSpy.count(), 1);
    QCOMPARE(completedSpy.takeFirst().at(0).toString(), QStringLiteral("abc"));
}

void ReactAgentCoreTests::stopSuppressesLateCallbacks() {
    ReactAgentCore core;
    MockProvider provider;
    core.setProvider(&provider);

    int chunkCount = 0;
    QObject::connect(&core, &ReactAgentCore::responseChunk, &core, [&](const StreamChunk&) {
        ++chunkCount;
    });

    QSignalSpy completedSpy(&core, &ReactAgentCore::completed);
    QSignalSpy errorSpy(&core, &ReactAgentCore::errorOccurred);

    core.run(QStringLiteral("hello"));
    QCOMPARE(provider.streamCalls, 1);

    core.stop();
    QVERIFY(provider.abortCalled);

    provider.emitChunk(QStringLiteral("late chunk"));
    provider.emitComplete();
    provider.emitError(QStringLiteral("late error"));

    QCOMPARE(chunkCount, 0);
    QCOMPARE(completedSpy.count(), 0);
    QCOMPARE(errorSpy.count(), 0);
}

void ReactAgentCoreTests::emptyStreamErrorUsesFallbackMessage() {
    ReactAgentCore core;
    MockProvider provider;
    core.setProvider(&provider);

    QSignalSpy errorSpy(&core, &ReactAgentCore::errorOccurred);
    core.run(QStringLiteral("hello"));
    QCOMPARE(provider.streamCalls, 1);

    provider.emitError(QString());

    QCOMPARE(errorSpy.count(), 1);
    QCOMPARE(errorSpy.takeFirst().at(0).toString(), QStringLiteral("模型流式调用失败"));
}

void ReactAgentCoreTests::noToolFirstIterationContinuesLoop() {
    ReactAgentCore core;
    MockProvider provider;
    core.setProvider(&provider);

    QSignalSpy completedSpy(&core, &ReactAgentCore::completed);

    core.run(QStringLiteral("hello"));
    QCOMPARE(provider.streamCalls, 1);

    // 第一轮无内容、无工具，不应立刻完成；应进入下一轮。
    provider.emitComplete();
    QTRY_COMPARE_WITH_TIMEOUT(provider.streamCalls, 2, 1000);
    QCOMPARE(completedSpy.count(), 0);

    provider.emitChunk(QStringLiteral("loop-ok"));
    provider.emitComplete();

    QCOMPARE(completedSpy.count(), 1);
    QCOMPARE(completedSpy.takeFirst().at(0).toString(), QStringLiteral("loop-ok"));
}

void ReactAgentCoreTests::invalidToolArgumentsBecomeToolMessage() {
    ReactAgentCore core;
    MockProvider provider;
    core.setProvider(&provider);

    MessageList latestMessages;
    QObject::connect(&core, &ReactAgentCore::conversationUpdated, &core, [&](const MessageList& messages) {
        latestMessages = messages;
    });

    QSignalSpy completedSpy(&core, &ReactAgentCore::completed);

    core.run(QStringLiteral("hello"));
    QCOMPARE(provider.streamCalls, 1);

    StreamChunk firstChunk;
    ToolCallDelta brokenTool;
    brokenTool.index = 0;
    brokenTool.id = QStringLiteral("tc-invalid");
    brokenTool.name = QStringLiteral("dummy_tool");
    brokenTool.arguments = QStringLiteral("{bad-json");
    firstChunk.toolCalls.append(brokenTool);
    provider.emitChunk(firstChunk);
    provider.emitComplete();

    QTRY_COMPARE_WITH_TIMEOUT(provider.streamCalls, 2, 1000);
    provider.emitChunk(QStringLiteral("final-after-tool-error"));
    provider.emitComplete();

    QCOMPARE(completedSpy.count(), 1);
    QCOMPARE(completedSpy.takeFirst().at(0).toString(), QStringLiteral("final-after-tool-error"));

    bool foundInvalidToolMessage = false;
    for (const Message& message : latestMessages) {
        if (message.role == MessageRole::Tool
            && message.content.contains(QStringLiteral("Invalid tool arguments JSON"))) {
            foundInvalidToolMessage = true;
            break;
        }
    }
    QVERIFY(foundInvalidToolMessage);
}

class ToolExecutorTests : public QObject {
    Q_OBJECT

private slots:
    void nestedPermissionConfirmationRejected();
    void nestedSubagentExecutionRejected();
};

void ToolExecutorTests::nestedPermissionConfirmationRejected() {
    PermissionConfig config;
    config.defaultAction = PermissionRule::Action::Ask;
    PermissionManager::instance().initialize(config);

    DummyTool tool(QStringLiteral("dummy_nested_permission"));
    ToolRegistry::instance().unregisterTool(tool.name());
    ToolRegistry::instance().registerTool(&tool);

    ToolExecutor executor(&ToolRegistry::instance(), &PermissionManager::instance());

    ToolResult nestedResult;
    QObject::connect(&executor, &ToolExecutor::permissionRequest, &executor,
                     [&](const ToolCall& call, std::function<void(bool)> callback) {
        Q_UNUSED(call);
        QTimer::singleShot(0, &executor, [&, callback = std::move(callback)]() mutable {
            ToolCall nestedCall;
            nestedCall.id = QStringLiteral("nested-perm");
            nestedCall.name = QStringLiteral("dummy_nested_permission");
            nestedResult = executor.execute(nestedCall);
            callback(false);
        });
    });

    ToolCall outerCall;
    outerCall.id = QStringLiteral("outer-perm");
    outerCall.name = QStringLiteral("dummy_nested_permission");
    ToolResult outerResult = executor.execute(outerCall);

    QVERIFY(!outerResult.success);
    QVERIFY(!nestedResult.success);
    QVERIFY(nestedResult.content.contains(QStringLiteral("nested blocking execution"), Qt::CaseInsensitive));

    ToolRegistry::instance().unregisterTool(tool.name());
}

void ToolExecutorTests::nestedSubagentExecutionRejected() {
    PermissionConfig config;
    config.defaultAction = PermissionRule::Action::Ask;
    PermissionManager::instance().initialize(config);

    DummyTool tool(QStringLiteral("dummy_nested_subagent"));
    ToolRegistry::instance().unregisterTool(tool.name());
    ToolRegistry::instance().registerTool(&tool);

    ToolExecutor executor(&ToolRegistry::instance(), &PermissionManager::instance());

    ToolResult nestedResult;
    QObject::connect(&executor, &ToolExecutor::permissionRequest, &executor,
                     [&](const ToolCall& call, std::function<void(bool)> callback) {
        Q_UNUSED(call);
        QTimer::singleShot(0, &executor, [&, callback = std::move(callback)]() mutable {
            ToolCall nestedSubagent;
            nestedSubagent.id = QStringLiteral("nested-subagent");
            nestedSubagent.name = QStringLiteral("start_subagent");
            nestedSubagent.arguments[QStringLiteral("task")] = QStringLiteral("inner task");
            nestedResult = executor.execute(nestedSubagent);
            callback(false);
        });
    });

    ToolCall outerCall;
    outerCall.id = QStringLiteral("outer-subagent");
    outerCall.name = QStringLiteral("dummy_nested_subagent");
    ToolResult outerResult = executor.execute(outerCall);

    QVERIFY(!outerResult.success);
    QVERIFY(!nestedResult.success);
    QVERIFY(nestedResult.content.contains(QStringLiteral("Nested subagent execution"), Qt::CaseInsensitive));

    ToolRegistry::instance().unregisterTool(tool.name());
}

class ConversationStoreTests : public QObject {
    Q_OBJECT

private slots:
    void saveLoadAndClearRoundTrip();
    void autoReconnectWhenDatabaseClosed();
};

void ConversationStoreTests::saveLoadAndClearRoundTrip() {
    QVERIFY(DatabaseManager::instance().initialize());

    ConversationStore store;
    const QString sessionId = QStringLiteral("test-session-") + QUuid::createUuid().toString(QUuid::WithoutBraces);

    MessageList messages;
    messages.append(Message(MessageRole::User, QStringLiteral("hello")));
    messages.append(Message(MessageRole::Assistant, QStringLiteral("world")));

    store.clearSession(sessionId);
    store.saveMessages(sessionId, messages);

    const MessageList loaded = store.loadMessages(sessionId);
    QCOMPARE(loaded.size(), 2);
    QCOMPARE(loaded.at(0).role, MessageRole::User);
    QCOMPARE(loaded.at(0).content, QStringLiteral("hello"));
    QCOMPARE(loaded.at(1).role, MessageRole::Assistant);
    QCOMPARE(loaded.at(1).content, QStringLiteral("world"));

    QVERIFY(store.sumTokens(sessionId) > 0);

    store.clearSession(sessionId);
    QCOMPARE(store.loadMessages(sessionId).size(), 0);
}

void ConversationStoreTests::autoReconnectWhenDatabaseClosed() {
    QVERIFY(DatabaseManager::instance().initialize());
    DatabaseManager::instance().close();

    ConversationStore store;
    const QString sessionId = QStringLiteral("test-reconnect-") + QUuid::createUuid().toString(QUuid::WithoutBraces);

    MessageList messages;
    messages.append(Message(MessageRole::User, QStringLiteral("reconnect-check")));

    store.saveMessages(sessionId, messages);
    const MessageList loaded = store.loadMessages(sessionId);
    QCOMPARE(loaded.size(), 1);
    QCOMPARE(loaded.at(0).content, QStringLiteral("reconnect-check"));

    store.clearSession(sessionId);
}

class HttpClientTests : public QObject {
    Q_OBJECT

private slots:
    void abortSuppressesStreamCallbacks();
};

void HttpClientTests::abortSuppressesStreamCallbacks() {
    QTcpServer server;
    QVERIFY(server.listen(QHostAddress::LocalHost, 0));

    int acceptedConnections = 0;
    QList<QTcpSocket*> sockets;
    QObject::connect(&server, &QTcpServer::newConnection, &server, [&]() {
        while (server.hasPendingConnections()) {
            QTcpSocket* socket = server.nextPendingConnection();
            if (!socket) {
                continue;
            }
            sockets.append(socket);
            ++acceptedConnections;
        }
    });

    HttpClient client;
    client.setBaseUrl(QStringLiteral("http://127.0.0.1:%1").arg(server.serverPort()));
    client.setApiKey(QStringLiteral("test-key"));
    client.setTimeout(3000);

    bool gotData = false;
    bool gotComplete = false;
    bool gotError = false;

    QJsonObject body;
    body[QStringLiteral("ping")] = QStringLiteral("pong");

    client.postStream(QStringLiteral("/stream"), body,
        [&](const QByteArray&) {
            gotData = true;
        },
        [&]() {
            gotComplete = true;
        },
        [&](const QString&) {
            gotError = true;
        });

    QTRY_VERIFY_WITH_TIMEOUT(acceptedConnections > 0, 2000);

    client.abort();
    QTest::qWait(200);

    QVERIFY(!gotData);
    QVERIFY(!gotComplete);
    QVERIFY(!gotError);

    for (QTcpSocket* socket : sockets) {
        if (socket) {
            socket->close();
            socket->deleteLater();
        }
    }
}

} // namespace clawpp

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);

    int status = 0;
    clawpp::ConversationHistoryTests historyTests;
    status |= QTest::qExec(&historyTests, argc, argv);

    clawpp::MemoryManagerTests memoryTests;
    status |= QTest::qExec(&memoryTests, argc, argv);

    clawpp::TemplateLoaderTests templateLoaderTests;
    status |= QTest::qExec(&templateLoaderTests, argc, argv);

    clawpp::PermissionManagerTests permissionTests;
    status |= QTest::qExec(&permissionTests, argc, argv);

    clawpp::ReactAgentCoreTests reactCoreTests;
    status |= QTest::qExec(&reactCoreTests, argc, argv);

    clawpp::ToolExecutorTests toolExecutorTests;
    status |= QTest::qExec(&toolExecutorTests, argc, argv);

    clawpp::ConversationStoreTests conversationStoreTests;
    status |= QTest::qExec(&conversationStoreTests, argc, argv);

    clawpp::HttpClientTests httpClientTests;
    status |= QTest::qExec(&httpClientTests, argc, argv);

    return status;
}

#include "test_core.moc"
