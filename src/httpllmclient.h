#pragma once

#include "illmclient.h"
#include "sseparser.h"

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>
#include <QSslSocket>

/**
 * 真实 LLM 客户端：封装 OpenAI 兼容 /chat/completions 的流式（SSE）与一次性请求。
 * 通过 ILlmClient 接口注入到 ShanHeBridge，从而可被替换 / 单测。
 *
 * 职责边界（相对初版把网络写进 bridge）：
 *  - 传输细节：URL 规整、鉴权头、SSE 解析、错误体兜底
 *  - 业务编排：留在 ShanHeBridge（构建 messages / system prompt / 进度）
 */
class HttpLlmClient : public QObject, public ILlmClient
{
    Q_OBJECT
public:
    explicit HttpLlmClient(QObject *parent = nullptr);

    /// 启动时检测 TLS 插件是否可用（缺失则 HTTPS 请求会静默失败）。
    static bool checkTlsAvailable();

    /// 注入连接配置（由 bridge 在 loadConfig / saveConfig 时调用）。
    void configure(const QString &apiBase, const QString &apiKey);
    /// [test-only] expose private normalizedChatUrl() for 7.7 regression test.
    QString normalizedChatUrlForTest() const { return normalizedChatUrl(); }

    void streamChat(const QJsonObject &payload,
                    std::function<void(const QString &)> onChunk,
                    std::function<void(bool, const QString &)> onDone) override;

    void complete(const QJsonObject &payload,
                  std::function<void(bool, const QString &)> onDone) override;

    void abort() override;
    bool isStreaming() const override;

signals:
    void tlsMissing();

private:
    QString normalizedChatUrl() const;

    QNetworkAccessManager m_net;
    QNetworkReply *m_reply = nullptr;
    SseParser m_sse;
    QString m_apiBase;
    QString m_apiKey;
};
