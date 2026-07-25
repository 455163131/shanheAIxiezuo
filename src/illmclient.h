#pragma once

#include <functional>
#include <optional>
#include <QString>
#include <QJsonObject>
#include "llmparams.h"

/**
 * LLM client abstract interface (Dependency Inversion).
 *
 * ShanHeBridge depends only on this interface; concrete implementations are
 * injected at runtime:
 *   - HttpLlmClient : real OpenAI-compatible /chat/completions streaming
 *   - MockLlmClient : built-in demo streaming when no API key is configured
 *   - FakeLlmClient : unit-test double (see tests/fakellmclient.h)
 *
 * Stage 2 adds generateWithControl: a controlled streaming state machine
 * (auto-continue + LeakGuard + timeout + error classification) used by
 * NovelGenerator's parallel three-way scheduling. The legacy
 * streamChat/complete/abort/isStreaming are kept for backward compatibility.
 */
class ILlmClient
{
public:
    virtual ~ILlmClient() = default;

    // ---- Legacy interface (backward compatible) ----

    /// Streaming chat: onChunk fires for each text delta; onDone(ok, error) at end.
    virtual void streamChat(const QJsonObject &payload,
                            std::function<void(const QString &)> onChunk,
                            std::function<void(bool, const QString &)> onDone) = 0;

    /// One-shot request (connectivity test): onDone(ok, message) at end.
    virtual void complete(const QJsonObject &payload,
                          std::function<void(bool, const QString &)> onDone) = 0;

    /// Abort the current streaming generation.
    virtual void abort() = 0;

    /// Whether a streaming generation is in progress.
    virtual bool isStreaming() const = 0;

    // ---- Stage 2: controlled streaming state machine ----

    /// Generation config (word count / sampling / messages / jobId).
    struct GenConfig {
        int wordCountMin = 0;
        int wordCountMax = 0;
        int maxTokens = 4000;
        LlmParams::SamplingFields sampling;
        bool emitThinking = false;
        QString systemMessage;
        QString userMessage;
        QString jobId;
    };

    /// Generation result (passed to onDone).
    struct GenResult {
        QString finishReason;       // "stop" / "error" / "aborted" / "length"
        int wordCount = 0;
        bool leakBlocked = false;
        int continueRounds = 0;
        QString reasoningFull;
    };

    /// Callback bundle.
    struct GenCallbacks {
        std::function<void(const QString &)> onDelta;       // text delta
        std::function<void(const QString &)> onThinking;    // reasoning delta (emitThinking=true)
        std::function<void(const QString &)> onMeta;        // phase: stream_wait / model_thinking / stream_done / auto_continue / error_*
        std::function<void(bool ok, const QString &err,
                           const QString &full,
                           const GenResult &result)> onDone;
    };

    /// Controlled streaming generation.
    /// @param cfg  generation config
    /// @param cb   callback bundle
    /// @param jobId job ID for abortJob routing (auto-generated when empty)
    virtual void generateWithControl(const GenConfig &cfg, const GenCallbacks &cb,
                                     const QString &jobId = QString()) = 0;

    /// Abort a specific job (only one of the three parallel ways).
    virtual void abortJob(const QString &jobId) = 0;

    /// Whether a specific job is still streaming.
    virtual bool isJobStreaming(const QString &jobId) const = 0;
};
