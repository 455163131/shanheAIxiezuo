#pragma once

#include "illmclient.h"
#include "sseparser.h"

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>
#include <QSslSocket>
#include <QHash>
#include <QTimer>
#include <QElapsedTimer>

/**
 * Real LLM client: OpenAI-compatible /chat/completions streaming (SSE) and
 * one-shot requests.
 *
 * Stage 2 adds the generateWithControl state machine:
 *  - streamOnce single-round streaming + auto-continue
 *    (needsContinue / looksAbruptCut / buildContinueUser)
 *  - LeakGuard detection: on hit, replaces output with BLOCK_MESSAGE
 *  - idle (90s) / total (240s) timeouts
 *  - 429 auto-retry once after 2s delay
 *  - classifyError categorizes 401/403/429/5xx/other
 *  - m_jobs QHash enables three-way parallel scheduling
 *    (jobId-routed abortJob / isJobStreaming)
 *
 * Legacy streamChat/complete/abort/isStreaming are kept for backward
 * compatibility (they use the shared m_reply / m_sse members).
 */
class HttpLlmClient : public QObject, public ILlmClient
{
    Q_OBJECT
public:
    explicit HttpLlmClient(QObject *parent = nullptr);

    /// Check TLS plugin availability at startup (missing -> HTTPS silently fails).
    static bool checkTlsAvailable();

    /// Inject connection config (called by bridge on loadConfig / saveConfig).
    void configure(const QString &apiBase, const QString &apiKey);

    // ---- Legacy interface ----
    void streamChat(const QJsonObject &payload,
                    std::function<void(const QString &)> onChunk,
                    std::function<void(bool, const QString &)> onDone) override;
    void complete(const QJsonObject &payload,
                  std::function<void(bool, const QString &)> onDone) override;
    void abort() override;
    bool isStreaming() const override;

    // ---- Stage 2: generateWithControl state machine ----
    void generateWithControl(const GenConfig &cfg, const GenCallbacks &cb,
                             const QString &jobId = QString()) override;
    void abortJob(const QString &jobId) override;
    bool isJobStreaming(const QString &jobId) const override;

    /// [test-only] expose private normalizedChatUrl() for 7.7 regression test.
    QString normalizedChatUrlForTest() const { return normalizedChatUrl(); }

signals:
    void tlsMissing();

private:
    /// Per-job state machine data.
    struct JobState {
        QString jobId;
        QNetworkReply *reply = nullptr;
        QElapsedTimer lastActivity;
        QTimer *idleTimer = nullptr;
        QTimer *totalTimer = nullptr;
        QString full;
        QString reasoningFull;
        int continueRounds = 0;
        GenCallbacks callbacks;
        GenConfig config;
        bool cancelled = false;   // set by abortJob
        bool aborting = false;    // set by timeout paths before reply->abort()
        bool finished = false;    // set by finishJob (idempotency guard)
        QJsonObject messagesContext;  // accumulated messages for auto-continue
        QByteArray sseBuffer;         // per-job SSE buffer (NOT shared m_sse)
    };

    QString normalizedChatUrl() const;

    /// Single streaming round (state machine core).
    void streamOnce(JobState *job);

    /// Whether auto-continue is needed.
    bool needsContinue(const JobState *job) const;

    /// Detect mid-sentence abrupt cut.
    bool looksAbruptCut(const QString &text) const;

    /// Build the "please continue" user message.
    QString buildContinueUser(int remainWords) const;

    /// Classify HTTP error into a user-facing message.
    QString classifyError(int httpStatus, const QString &body) const;

    /// Unified finalize: stop timers -> onDone -> deleteLater(reply) ->
    /// remove from m_jobs -> delete job. Idempotent via `finished` flag.
    void finishJob(JobState *job, bool ok, const QString &err,
                   const QString &full, bool leakBlocked = false);

    QNetworkAccessManager m_net;
    QNetworkReply *m_reply = nullptr;  // legacy streamChat
    SseParser m_sse;                   // legacy streamChat
    QString m_apiBase;
    QString m_apiKey;

    /// jobId -> JobState mapping (three-way parallel support).
    QHash<QString, JobState *> m_jobs;
};
