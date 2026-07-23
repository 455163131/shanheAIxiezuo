#include "httpllmclient.h"

HttpLlmClient::HttpLlmClient(QObject *parent)
    : QObject(parent)
    , m_net(this)
{
}

void HttpLlmClient::configure(const QString &apiBase, const QString &apiKey)
{
    m_apiBase = apiBase.trimmed();
    m_apiKey  = apiKey.trimmed();
}

QString HttpLlmClient::normalizedChatUrl() const
{
    QString base = m_apiBase;
    while (base.endsWith(QLatin1Char('/')))
        base.chop(1);
    // 用户可填 https://api.openai.com/v1 或已含 /chat/completions
    if (base.endsWith(QStringLiteral("/chat/completions")))
        return base;
    if (base.endsWith(QStringLiteral("/v1")))
        return base + QStringLiteral("/chat/completions");
    // 兜底：直接拼 /v1/chat/completions
    return base + QStringLiteral("/v1/chat/completions");
}

void HttpLlmClient::streamChat(const QJsonObject &payload,
                               std::function<void(const QString &)> onChunk,
                               std::function<void(bool, const QString &)> onDone)
{
    // 取消任何进行中的请求（避免上一次流未结束时又发起新请求）
    abort();
    m_sse.reset();

    QNetworkRequest req{QUrl(normalizedChatUrl())};
    req.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    req.setRawHeader("Authorization", QByteArray("Bearer ") + m_apiKey.toUtf8());
    req.setRawHeader("Accept", "text/event-stream");

    QNetworkReply *reply = m_net.post(req, QJsonDocument(payload).toJson(QJsonDocument::Compact));
    m_reply = reply;

    QObject::connect(reply, &QNetworkReply::readyRead, this, [this, reply, onChunk]() {
        if (reply != m_reply) return;            // 已被更新的请求取代
        const QList<QString> deltas = m_sse.feed(reply->readAll());
        for (const QString &d : deltas) {
            if (onChunk) onChunk(d);
        }
    });

    QObject::connect(reply, &QNetworkReply::finished, this, [this, reply, onDone]() {
        if (reply != m_reply) {                  // 过期回复：仅清理，避免回调错乱
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
