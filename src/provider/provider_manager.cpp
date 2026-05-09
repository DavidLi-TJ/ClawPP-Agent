#include "provider_manager.h"
#include <QCryptographicHash>
#include <QDateTime>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <utility>

namespace clawpp {

ProviderManager::ProviderManager(QObject* parent)
    : ILLMProvider(parent) {}

ProviderManager& ProviderManager::instance() {
    static ProviderManager instance;
    return instance;
}

QString ProviderManager::name() const {
    return m_defaultProvider.isEmpty() ? QStringLiteral("provider_manager") : m_defaultProvider;
}

bool ProviderManager::isAvailable() const {
    auto* provider = getDefault();
    return provider && provider->isAvailable();
}

QString ProviderManager::lastError() const {
    if (auto* provider = getDefault()) {
        return provider->lastError();
    }
    return m_lastError;
}

bool ProviderManager::supportsStreaming() const {
    if (auto* provider = getDefault()) {
        return provider->supportsStreaming();
    }
    return false;
}

bool ProviderManager::supportsTools() const {
    if (auto* provider = getDefault()) {
        return provider->supportsTools();
    }
    return false;
}

void ProviderManager::abort() {
    if (auto* provider = getDefault()) {
        provider->abort();
    }
}

void ProviderManager::setConfig(const ProviderConfig& config) {
    m_config = config;
    pruneExpiredPromptCache();
    initialize(config);
}

void ProviderManager::initialize(const ProviderConfig& config) {
    m_config = config;
    const QString requestedProvider = normalizeProviderId(config.type);
    const QString previousDefault = m_defaultProvider;

    // 默认维护三类 provider，便于随时切换。
    const QStringList builtins = {
        QStringLiteral("openai"),
        QStringLiteral("anthropic"),
        QStringLiteral("gemini")
    };

    for (const QString& id : builtins) {
        ILLMProvider* provider = createProvider(id, configForProvider(id, config));
        registerProvider(id, provider);
    }

    const QString target = hasProvider(requestedProvider)
        ? requestedProvider
        : (hasProvider(previousDefault) ? previousDefault : QStringLiteral("openai"));
    setDefaultProvider(target);
}

void ProviderManager::registerProvider(const QString& providerId, ILLMProvider* provider) {
    const QString key = normalizeProviderId(providerId);
    if (key.isEmpty() || !provider) {
        return;
    }

    ILLMProvider* existing = m_providers.value(key, nullptr);
    if (existing == provider) {
        return;
    }

    if (existing && existing->parent() == this) {
        existing->deleteLater();
    }

    if (!provider->parent()) {
        provider->setParent(this);
    }

    m_providers[key] = provider;
    if (m_defaultProvider.isEmpty()) {
        m_defaultProvider = key;
    }
}

bool ProviderManager::setDefaultProvider(const QString& providerId) {
    const QString key = normalizeProviderId(providerId);
    if (m_providers.contains(key)) {
        m_defaultProvider = key;
        m_lastError.clear();
        return true;
    } else {
        m_lastError = QStringLiteral("Provider not found: %1").arg(providerId);
        return false;
    }
}

bool ProviderManager::hasProvider(const QString& providerId) const {
    const QString key = normalizeProviderId(providerId);
    return m_providers.contains(key);
}

QString ProviderManager::currentProviderId() const {
    return m_defaultProvider;
}

QStringList ProviderManager::providerIds() const {
    return m_providers.keys();
}

ILLMProvider* ProviderManager::getProvider(const QString& providerId) const {
    QString key = providerId.isEmpty() ? m_defaultProvider : normalizeProviderId(providerId);
    return m_providers.value(key, nullptr);
}

ILLMProvider* ProviderManager::getDefault() const {
    return getProvider(m_defaultProvider);
}

QStringList ProviderManager::listAvailable() const {
    QStringList result;
    for (auto it = m_providers.begin(); it != m_providers.end(); ++it) {
        if (it.value() && it.value()->isAvailable()) {
            result.append(it.key());
        }
    }
    return result;
}

void ProviderManager::chat(const MessageList& messages,
                           const ChatOptions& options,
                           std::function<void(const QString&)> onSuccess,
                           std::function<void(const QString&)> onError) {
    chatWithProvider(QString(), messages, options, std::move(onSuccess), std::move(onError));
}

void ProviderManager::chatWithProvider(const QString& providerId,
                                       const MessageList& messages,
                                       const ChatOptions& options,
                                       std::function<void(const QString&)> onSuccess,
                                       std::function<void(const QString&)> onError) {
    auto* provider = getProvider(providerId);
    if (!provider) {
        m_lastError = QStringLiteral("No provider available");
        onError(m_lastError);
        return;
    }

    const QString effectiveProvider = providerId.isEmpty() ? m_defaultProvider : normalizeProviderId(providerId);
    const bool cacheable = isCacheableRequest(messages, options);
    const QString cacheKey = cacheable ? buildPromptCacheKey(effectiveProvider, messages, options) : QString();

    if (cacheable) {
        const QString hit = getCachedPromptResponse(cacheKey);
        if (!hit.isNull()) {
            onSuccess(hit);
            return;
        }
    }

    provider->chat(messages, options,
                   [this, onSuccess, cacheable, cacheKey](const QString& response) {
                       if (cacheable && !response.isNull()) {
                           putCachedPromptResponse(cacheKey, response);
                       }
                       onSuccess(response);
                   },
                   [onError](const QString& error) {
                       onError(error);
                   });
}

void ProviderManager::chatStream(const MessageList& messages,
                                 const ChatOptions& options,
                                 StreamCallback onToken,
                                 std::function<void()> onComplete,
                                 ErrorCallback onError) {
    chatStreamWithProvider(QString(), messages, options, std::move(onToken), std::move(onComplete), std::move(onError));
}

void ProviderManager::chatStreamWithProvider(const QString& providerId,
                                             const MessageList& messages,
                                             const ChatOptions& options,
                                             StreamCallback onToken,
                                             std::function<void()> onComplete,
                                             ErrorCallback onError) {
    auto* provider = getProvider(providerId);
    if (provider) {
        if (!provider->supportsStreaming()) {
            ChatOptions fallback = options;
            fallback.stream = false;
            provider->chat(
                messages,
                fallback,
                [onToken, onComplete](const QString& response) {
                    if (!response.isEmpty()) {
                        StreamChunk chunk;
                        chunk.content = response;
                        onToken(chunk);
                    }
                    onComplete();
                },
                onError);
            return;
        }
        provider->chatStream(messages, options, onToken, onComplete, onError);
    } else {
        m_lastError = QStringLiteral("No provider available");
        onError(m_lastError);
    }
}

QString ProviderManager::normalizeProviderId(const QString& providerId) const {
    QString id = providerId.trimmed().toLower();
    if (id.isEmpty()) {
        return QStringLiteral("openai");
    }
    if (id == QStringLiteral("claude")) {
        return QStringLiteral("anthropic");
    }
    if (id == QStringLiteral("google")) {
        return QStringLiteral("gemini");
    }
    return id;
}

ILLMProvider* ProviderManager::createProvider(const QString& providerId, const ProviderConfig& config) {
    const QString id = normalizeProviderId(providerId);
    if (id == QStringLiteral("anthropic")) {
        return new AnthropicProvider(config, this);
    }
    if (id == QStringLiteral("gemini")) {
        return new GeminiProvider(config, this);
    }
    return new OpenAIProvider(config, this);
}

ProviderConfig ProviderManager::configForProvider(const QString& providerId, const ProviderConfig& base) const {
    ProviderConfig out = base;
    const QString id = normalizeProviderId(providerId);
    out.type = id;
    if (id == QStringLiteral("anthropic")) {
        if (out.baseUrl.trimmed().isEmpty() || out.baseUrl == QStringLiteral("https://api.openai.com/v1")) {
            out.baseUrl = QStringLiteral("https://api.anthropic.com");
        }
    } else if (id == QStringLiteral("gemini")) {
        if (out.baseUrl.trimmed().isEmpty() || out.baseUrl == QStringLiteral("https://api.openai.com/v1")) {
            out.baseUrl = QStringLiteral("https://generativelanguage.googleapis.com/v1beta");
        }
    } else {
        if (out.baseUrl.trimmed().isEmpty()) {
            out.baseUrl = QStringLiteral("https://api.openai.com/v1");
        }
    }
    return out;
}

bool ProviderManager::isCacheableRequest(const MessageList& messages, const ChatOptions& options) const {
    if (options.stream) {
        return false;
    }
    if (!options.tools.isEmpty()) {
        return false;
    }
    if (messages.isEmpty()) {
        return false;
    }
    if (messages.size() > 12) {
        return false;
    }

    for (const Message& msg : messages) {
        if (msg.role == MessageRole::Tool) {
            return false;
        }
        if (!msg.toolCalls.isEmpty()) {
            return false;
        }
    }
    return true;
}

QString ProviderManager::buildPromptCacheKey(const QString& providerId,
                                             const MessageList& messages,
                                             const ChatOptions& options) const {
    QJsonObject root;
    root.insert(QStringLiteral("provider"), providerId);
    root.insert(QStringLiteral("model"), options.model.isEmpty() ? m_config.model : options.model);
    root.insert(QStringLiteral("temperature"), options.temperature);
    root.insert(QStringLiteral("max_tokens"), options.maxTokens);
    root.insert(QStringLiteral("system_prompt"), options.systemPrompt);

    QJsonArray rows;
    for (const Message& msg : messages) {
        QJsonObject row;
        row.insert(QStringLiteral("role"), static_cast<int>(msg.role));
        row.insert(QStringLiteral("content"), msg.content);
        row.insert(QStringLiteral("name"), msg.name);
        row.insert(QStringLiteral("tool_call_id"), msg.toolCallId);
        rows.append(row);
    }
    root.insert(QStringLiteral("messages"), rows);

    const QByteArray keyBytes = QJsonDocument(root).toJson(QJsonDocument::Compact);
    return QString::fromLatin1(QCryptographicHash::hash(keyBytes, QCryptographicHash::Sha256).toHex());
}

QString ProviderManager::getCachedPromptResponse(const QString& key) {
    if (key.isEmpty()) {
        return QString();
    }
    pruneExpiredPromptCache();
    auto it = m_promptCache.find(key);
    if (it == m_promptCache.end()) {
        return QString();
    }
    return it->response;
}

void ProviderManager::putCachedPromptResponse(const QString& key, const QString& response) {
    if (key.isEmpty()) {
        return;
    }
    if (response.isEmpty()) {
        return;
    }

    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    PromptCacheEntry entry;
    entry.response = response;
    entry.expiresAtMs = now + m_promptCacheTtlMs;

    const bool existed = m_promptCache.contains(key);
    m_promptCache.insert(key, entry);
    if (!existed) {
        m_promptCacheOrder.enqueue(key);
    }

    while (m_promptCache.size() > m_promptCacheMaxEntries && !m_promptCacheOrder.isEmpty()) {
        const QString oldest = m_promptCacheOrder.dequeue();
        if (oldest != key) {
            m_promptCache.remove(oldest);
        }
    }
    pruneExpiredPromptCache();
}

void ProviderManager::pruneExpiredPromptCache() {
    if (m_promptCache.isEmpty()) {
        return;
    }
    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    while (!m_promptCacheOrder.isEmpty()) {
        const QString& head = m_promptCacheOrder.head();
        auto it = m_promptCache.find(head);
        if (it == m_promptCache.end()) {
            m_promptCacheOrder.dequeue();
            continue;
        }
        if (it->expiresAtMs > now) {
            break;
        }
        m_promptCache.remove(head);
        m_promptCacheOrder.dequeue();
    }
}

}
