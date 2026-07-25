#include "httpllmclient.h"
#include "textutils.h"
#include "leakguard.h"
#include "llmparams.h"

#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QDateTime>
#include <QTimer>
#include <QElapsedTimer>

HttpLlmClient::HttpLlmClient(QObject *parent)
    : QObject(parent)
    , m_net(this)
{
}

bool HttpLlmClient::checkTlsAvailable()
{
    return QSslSocket::supportsSsl();
}

void HttpLlmClient::configure(const QString &apiBase, const QString &apiKey)
{
    m_apiBase = apiBase.trimmed();
    m_apiKey  = apiKey.trimmed();
}

QString HttpLlmClient::normalizedChatUrl() const
{
    QString base = m_apiBase;
    while (base.endsWith(QLatin1Char('/'))) base.chop(1);
    const QString suffix = QStringLiteral("/chat/completions");
    if (base.endsWith(suffix, Qt::CaseInsensitive)) return base;
    return base + suffix;
}

// ---------------------------------------------------------------------------
// Legacy streamChat (kept for backward compatibility)
// ---------------------------------------------------------------------------
void HttpLlmClient::streamChat(const QJsonObject &payload,
                               std::function<void(const QString &)> onChunk,
                               std::function<void(bool, const QString &)> onDone)
{
    abort();
    m_sse.reset();

    QNetworkRequest req{QUrl(normalizedChatUrl())};
    req.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    req.setRawHeader("Authorization", QByteArray("Bearer ") + m_apiKey.toUtf8());
    req.setRawHeader("Accept", "text/event-stream");

    QNetworkReply *reply = m_net.post(req, QJsonDocument(payload).toJson(QJsonDocument::Compact));
    m_reply = reply;

    QObject::connect(reply, &QNetworkReply::readyRead, this, [this, reply, onChunk]() {
        if (reply != m_reply) return;
        const QList<QString> deltas = m_sse.feed(reply->readAll());
        for (const QString &d : deltas) {
            if (onChunk) onChunk(d);
        }
    });

    QObject::connect(reply, &QNetworkReply::finished, this, [this, reply, onDone]() {
        if (reply != m_reply) {
            reply->deleteLater();
            return;
        }
        const bool cancelled = (reply->error() == QNetworkReply::OperationCanceledError);
        const int http = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        const QByteArray body = reply->readAll();
        const bool ok = (reply->error() == QNetworkReply::NoError);

        QString err;
        if (!ok && !cancelled) {
            err = reply->errorString();
            const QJsonDocument ed = QJsonDocument::fromJson(m_sse.buffer() + body);
            if (ed.isObject()) {
                const QJsonObject eo = ed.object().value(QStringLiteral("error")).toObject();
                if (!eo.isEmpty())
                    err = eo.value(QStringLiteral("message")).toString(err);
            }
        }

        reply->deleteLater();
        m_reply = nullptr;

        if (!onDone) return;
        if (cancelled)
            onDone(false, QStringLiteral("cancelled"));
        else if (ok)
            onDone(true, QString());
        else
            onDone(false, QStringLiteral("API 调用失败 (HTTP %1)：%2").arg(http).arg(err));
    });
}

void HttpLlmClient::complete(const QJsonObject &payload,
                             std::function<void(bool, const QString &)> onDone)
{
    QNetworkRequest req{QUrl(normalizedChatUrl())};
    req.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    req.setRawHeader("Authorization", QByteArray("Bearer ") + m_apiKey.toUtf8());

    QNetworkReply *r = m_net.post(req, QJsonDocument(payload).toJson(QJsonDocument::Compact));
    QObject::connect(r, &QNetworkReply::finished, this, [r, onDone, payload]() {
        const int http = r->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        const QByteArray body = r->readAll();
        const bool ok = (r->error() == QNetworkReply::NoError) && http >= 200 && http < 300;

        QString msg;
        if (ok) {
            msg = QStringLiteral("连接成功（模型 %1 可用）").arg(payload.value(QStringLiteral("model")).toString());
        } else {
            msg = r->errorString();
            const QJsonDocument ed = QJsonDocument::fromJson(body);
            if (ed.isObject()) {
                const QJsonObject eo = ed.object().value(QStringLiteral("error")).toObject();
                if (!eo.isEmpty())
                    msg = eo.value(QStringLiteral("message")).toString(msg);
            }
            msg = QStringLiteral("HTTP %1：%2").arg(http).arg(msg);
        }
        r->deleteLater();
        if (onDone) onDone(ok, msg);
    });
}

void HttpLlmClient::abort()
{
    if (m_reply) m_reply->abort();
}

bool HttpLlmClient::isStreaming() const
{
    return m_reply != nullptr;
}

// ===========================================================================
// Stage 2: generateWithControl state machine
// ===========================================================================

void HttpLlmClient::generateWithControl(const GenConfig &cfg, const GenCallbacks &cb,
                                        const QString &jobId)
{
    QString id = jobId.isEmpty()
                     ? QStringLiteral("single_%1").arg(QDateTime::currentMSecsSinceEpoch())
                     : jobId;

    // If a job with the same id exists, abort it first.
    if (m_jobs.contains(id)) {
        abortJob(id);
    }

    auto *job = new JobState();
    job->jobId = id;
    job->callbacks = cb;
    job->config = cfg;
    m_jobs.insert(id, job);

    // Initialize messages context.
    QJsonArray messages;
    if (!cfg.systemMessage.isEmpty()) {
        QJsonObject sysMsg;
        sysMsg["role"] = QStringLiteral("system");
        sysMsg["content"] = cfg.systemMessage;
        messages.append(sysMsg);
    }
    QJsonObject userMsg;
    userMsg["role"] = QStringLiteral("user");
    userMsg["content"] = cfg.userMessage;
    messages.append(userMsg);
    job->messagesContext["messages"] = messages;

    streamOnce(job);
}

void HttpLlmClient::streamOnce(JobState *job)
{
    if (job->cancelled || job->finished) return;

    // ---- Build request payload ----
    QJsonObject payload;
    payload["model"] = QStringLiteral("");  // bridge fills this in configure
    payload["stream"] = true;

    // max_tokens: first round slack=1.4, continue slack=1.5
    const int targetWords = job->config.wordCountMax;
    const int thinkingBudget = job->config.sampling.thinkingBudget.value_or(0);
    const int userMaxTokens = job->config.maxTokens;
    const double slack = (job->continueRounds == 0) ? 1.4 : 1.5;
    const int maxTokens = LlmParams::tokensForWordBudget(
        targetWords, thinkingBudget, userMaxTokens, slack);
    payload["max_tokens"] = maxTokens;

    // sampling fields
    const auto &s = job->config.sampling;
    if (s.temperature >= 0) {  // -1 sentinel: skip (reasoner softening)
        payload["temperature"] = s.temperature;
    }
    if (s.reasoningEffort.has_value()) {
        payload["reasoning_effort"] = s.reasoningEffort.value();
    }
    if (s.thinkingBudget.has_value()) {
        payload["thinking_budget"] = s.thinkingBudget.value();
    }

    payload["messages"] = job->messagesContext["messages"].toArray();

    // Clean up any prior reply (429 retry path).
    if (job->reply) {
        job->reply->deleteLater();
        job->reply = nullptr;
    }
    job->sseBuffer.clear();
    job->aborting = false;

    // ---- Issue request ----
    QNetworkRequest req{QUrl(normalizedChatUrl())};
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    req.setRawHeader("Authorization",
                     QStringLiteral("Bearer %1").arg(m_apiKey).toUtf8());
    req.setRawHeader("Accept", "text/event-stream");

    job->reply = m_net.post(req, QJsonDocument(payload).toJson(QJsonDocument::Compact));
    job->lastActivity.start();

    // ---- idle timer (5s tick, 90s no-activity -> timeout) ----
    if (!job->idleTimer) {
        job->idleTimer = new QTimer(this);
        job->idleTimer->setSingleShot(false);
        connect(job->idleTimer, &QTimer::timeout, this, [this, job]() {
            if (job->finished || job->aborting) return;
            if (job->lastActivity.hasExpired(90000)) {
                // Set aborting BEFORE abort() so a synchronously-fired
                // finished lambda returns early and does not call finishJob.
                job->aborting = true;
                job->idleTimer->stop();
                if (job->reply) job->reply->abort();
                if (job->callbacks.onMeta)
                    job->callbacks.onMeta(QStringLiteral("error_timeout"));
                finishJob(job, false,
                          QStringLiteral("请求超时（idle 90s）"), job->full);
            }
        });
    }
    job->idleTimer->start(5000);

    // ---- total timer (240s hard cap) ----
    if (!job->totalTimer) {
        job->totalTimer = new QTimer(this);
        job->totalTimer->setSingleShot(true);
        connect(job->totalTimer, &QTimer::timeout, this, [this, job]() {
            if (job->finished || job->aborting) return;
            job->aborting = true;
            if (job->reply) job->reply->abort();
            if (job->callbacks.onMeta)
                job->callbacks.onMeta(QStringLiteral("error_timeout"));
            finishJob(job, false,
                      QStringLiteral("请求超时（total 240s）"), job->full);
        });
    }
    job->totalTimer->start(240000);

    // ---- model_thinking phase (5s no-output hint) ----
    QString thinkingJobId = job->jobId;
    QTimer::singleShot(5000, this, [this, thinkingJobId]() {
        auto it = m_jobs.find(thinkingJobId);
        if (it == m_jobs.end()) return;
        JobState *j = it.value();
        if (j->finished || j->cancelled) return;
        if (j->full.isEmpty() && j->callbacks.onMeta)
            j->callbacks.onMeta(QStringLiteral("model_thinking"));
    });

    // ---- stream_wait phase ----
    if (job->callbacks.onMeta)
        job->callbacks.onMeta(QStringLiteral("stream_wait"));

    // ---- SSE readyRead: per-job buffer, NOT shared m_sse ----
    connect(job->reply, &QIODevice::readyRead, this, [this, job]() {
        if (job->finished) return;
        job->lastActivity.restart();
        job->sseBuffer.append(job->reply->readAll());

        // Parse complete lines (handles TCP fragmentation).
        int idx;
        while ((idx = job->sseBuffer.indexOf('\n')) != -1) {
            QByteArray line = job->sseBuffer.left(idx);
            job->sseBuffer.remove(0, idx + 1);
            QByteArray trimmed = line.trimmed();
            if (trimmed.isEmpty()) continue;
            if (!trimmed.startsWith("data:")) continue;
            QByteArray data = trimmed.mid(5).trimmed();
            if (data.isEmpty() || data == "[DONE]") continue;

            QJsonParseError perr;
            const QJsonDocument d = QJsonDocument::fromJson(data, &perr);
            if (perr.error != QJsonParseError::NoError || !d.isObject()) continue;
            const QJsonObject chunk = d.object();
            const QJsonArray choices = chunk.value("choices").toArray();
            if (choices.isEmpty()) continue;
            const QJsonObject choice = choices.at(0).toObject();
            const QJsonObject delta = choice.value("delta").toObject();

            // Content delta
            const QString content = delta.value("content").toString();
            if (!content.isEmpty()) {
                job->full += content;
                if (job->callbacks.onDelta) job->callbacks.onDelta(content);
            }

            // Reasoning delta (only when emitThinking=true)
            const QString reasoning = delta.value("reasoning_content").toString();
            if (!reasoning.isEmpty() && job->config.emitThinking) {
                job->reasoningFull += reasoning;
                if (job->callbacks.onThinking) job->callbacks.onThinking(reasoning);
            }
        }
    });

    // ---- finished handler (state machine transitions) ----
    connect(job->reply, &QNetworkReply::finished, this, [this, job]() {
        if (job->finished) return;
        // If a timeout path already initiated abort, it owns finishJob.
        if (job->aborting) return;

        job->idleTimer->stop();
        job->totalTimer->stop();

        if (job->cancelled) {
            finishJob(job, false, QStringLiteral("aborted"), job->full);
            return;
        }

        const QNetworkReply::NetworkError err = job->reply->error();
        const int httpStatus = job->reply->attribute(
            QNetworkRequest::HttpStatusCodeAttribute).toInt();

        // Network-level error (no HTTP status)
        if (err != QNetworkReply::NoError && httpStatus == 0) {
            if (job->callbacks.onMeta)
                job->callbacks.onMeta(QStringLiteral("error_network"));
            finishJob(job, false,
                      QStringLiteral("网络错误：%1").arg(job->reply->errorString()),
                      job->full);
            return;
        }

        // HTTP error
        if (httpStatus >= 400) {
            const QString errBody = job->reply->readAll();
            const QString errMsg = classifyError(httpStatus, errBody);
            const QString phase = QStringLiteral("error_%1").arg(httpStatus);
            if (job->callbacks.onMeta) job->callbacks.onMeta(phase);

            // 429: retry once after 2s (only on first round)
            if (httpStatus == 429 && job->continueRounds == 0) {
                job->continueRounds++;
                QString retryJobId = job->jobId;
                QTimer::singleShot(2000, this, [this, retryJobId]() {
                    auto it = m_jobs.find(retryJobId);
                    if (it == m_jobs.end()) return;
                    streamOnce(it.value());
                });
                return;
            }

            finishJob(job, false, errMsg, job->full);
            return;
        }

        // Normal completion
        if (job->callbacks.onMeta)
            job->callbacks.onMeta(QStringLiteral("stream_done"));

        // LeakGuard detection
        if (LeakGuard::looksLikePromptLeak(job->full, job->config.userMessage)) {
            if (job->callbacks.onMeta)
                job->callbacks.onMeta(QStringLiteral("leak_blocked"));
            finishJob(job, false, LeakGuard::BLOCK_MESSAGE,
                      LeakGuard::BLOCK_MESSAGE, true);
            return;
        }

        // Auto-continue check
        if (needsContinue(job)) {
            job->continueRounds++;
            if (job->continueRounds > 2) {
                finishJob(job, true, QString(), job->full);
                return;
            }

            if (job->callbacks.onMeta)
                job->callbacks.onMeta(QStringLiteral("auto_continue"));

            // Append assistant message
            QJsonArray msgs = job->messagesContext["messages"].toArray();
            QJsonObject assistantMsg;
            assistantMsg["role"] = QStringLiteral("assistant");
            assistantMsg["content"] = job->full;
            msgs.append(assistantMsg);

            // Append continue user message
            const int remainWords = job->config.wordCountMax
                                    - TextUtils::countWords(job->full);
            QJsonObject continueMsg;
            continueMsg["role"] = QStringLiteral("user");
            continueMsg["content"] = buildContinueUser(remainWords);
            msgs.append(continueMsg);
            job->messagesContext["messages"] = msgs;

            // Continue rounds don't think
            job->config.sampling.thinkingBudget = 0;
            job->config.sampling.reasoningEffort = std::nullopt;

            streamOnce(job);
            return;
        }

        finishJob(job, true, QString(), job->full);
    });
}

bool HttpLlmClient::needsContinue(const JobState *job) const
{
    const int wordCount = TextUtils::countWords(job->full);
    const int wordMin = job->config.wordCountMin;
    const int wordMax = job->config.wordCountMax;

    // Met the upper bound -> stop
    if (wordCount >= wordMax) return false;

    // Below the lower bound -> must continue
    if (wordCount < wordMin) return true;

    // Abrupt cut + remaining >= 200 -> continue
    if (looksAbruptCut(job->full) && (wordMax - wordCount) >= 200) {
        return true;
    }
    return false;
}

bool HttpLlmClient::looksAbruptCut(const QString &text) const
{
    if (text.length() < 80) return false;
    const QChar last = text.at(text.length() - 1);
    static const QString endings = QStringLiteral("。！？…」』》》）)]】");
    return !endings.contains(last);
}

QString HttpLlmClient::buildContinueUser(int remainWords) const
{
    return QStringLiteral(
        "请继续续写，还需约%1字。要求：\n"
        "1. 紧接上文最后一句继续，不要重写已有内容。\n"
        "2. 不要添加任何引导语或说明。\n"
        "3. 在完整句号处收束。"
    ).arg(remainWords);
}

QString HttpLlmClient::classifyError(int httpStatus, const QString &body) const
{
    if (httpStatus == 401 || httpStatus == 403) {
        return QStringLiteral("API Key 无效或权限不足（HTTP %1）").arg(httpStatus);
    }
    if (httpStatus == 429) {
        return QStringLiteral("请求频率超限（HTTP 429），已自动重试 1 次");
    }
    if (httpStatus >= 500) {
        return QStringLiteral("服务器错误（HTTP %1）：%2")
            .arg(httpStatus)
            .arg(body.left(200));
    }
    return QStringLiteral("请求失败（HTTP %1）：%2")
        .arg(httpStatus)
        .arg(body.left(200));
}

void HttpLlmClient::finishJob(JobState *job, bool ok, const QString &err,
                              const QString &full, bool leakBlocked)
{
    if (job->finished) return;  // idempotent
    job->finished = true;

    if (job->idleTimer) {
        job->idleTimer->stop();
        job->idleTimer->deleteLater();
        job->idleTimer = nullptr;
    }
    if (job->totalTimer) {
        job->totalTimer->stop();
        job->totalTimer->deleteLater();
        job->totalTimer = nullptr;
    }

    GenResult result;
    result.finishReason = ok ? QStringLiteral("stop") : QStringLiteral("error");
    if (!ok && err == QStringLiteral("aborted"))
        result.finishReason = QStringLiteral("aborted");
    result.wordCount = TextUtils::countWords(full);
    result.leakBlocked = leakBlocked;
    result.continueRounds = job->continueRounds;
    result.reasoningFull = job->reasoningFull;

    if (job->callbacks.onDone)
        job->callbacks.onDone(ok, err, full, result);

    if (job->reply) {
        job->reply->deleteLater();
        job->reply = nullptr;
    }
    m_jobs.remove(job->jobId);
    delete job;
}

void HttpLlmClient::abortJob(const QString &jobId)
{
    auto it = m_jobs.find(jobId);
    if (it == m_jobs.end()) return;

    JobState *job = it.value();
    if (job->cancelled || job->finished) return;  // idempotent
    job->cancelled = true;

    if (job->reply && job->reply->isRunning()) {
        // abort() may synchronously fire finished; the finished lambda sees
        // cancelled=true and calls finishJob. Do NOT touch `job` after this.
        job->reply->abort();
    } else {
        // No active reply (e.g. during 429 retry wait) -> finalize directly.
        finishJob(job, false, QStringLiteral("aborted"), job->full);
    }
}

bool HttpLlmClient::isJobStreaming(const QString &jobId) const
{
    return m_jobs.contains(jobId);
}
