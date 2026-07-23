#pragma once

#include "illmclient.h"

#include <QObject>
#include <QTimer>
#include <QString>

/**
 * 内置演示客户端：未配置 API 或 backend=="mock" 时使用，模拟流式输出，
 * 让全链路在无密钥情况下也能跑通。它是 ILlmClient 的一种实现，
 * 把初版写在 ShanHeBridge 里的 mock 定时器逻辑独立出来。
 */
class MockLlmClient : public QObject, public ILlmClient
{
    Q_OBJECT
public:
    explicit MockLlmClient(QObject *parent = nullptr);

    void streamChat(const QJsonObject &payload,
                    std::function<void(const QString &)> onChunk,
                    std::function<void(bool, const QString &)> onDone) override;

    void complete(const QJsonObject &payload,
                  std::function<void(bool, const QString &)> onDone) override;

    void abort() override;
    bool isStreaming() const override;

private slots:
    void tick();

private:
    QString buildScriptedText(const QJsonObject &payload) const;

    QTimer *m_timer = nullptr;
    QString m_full;
    int m_pos = 0;
    std::function<void(const QString &)> m_onChunk;
    std::function<void(bool, const QString &)> m_onDone;
};
