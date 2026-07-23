#pragma once

#include "illmclient.h"

#include <QJsonObject>

/**
 * 单元测试替身（Test Double）：脚本化地驱动流式输出，用于验证
 * ShanHeBridge 的编排逻辑，而无需真实网络或密钥。
 *
 * 用法：设置 script 回调，在收到 streamChat 时自行调用 onChunk / onDone。
 */
class FakeLlmClient : public ILlmClient
{
public:
    bool streaming = false;
    bool aborted = false;
    int streamChatCalls = 0;
    int completeCalls = 0;
    QJsonObject lastPayload;

    // 由测试注入：收到 streamChat 时调用 script(payload, onChunk, onDone)
    std::function<void(const QJsonObject &,
                       std::function<void(const QString &)>,
                       std::function<void(bool, const QString &)>)> script;

    void streamChat(const QJsonObject &payload,
                    std::function<void(const QString &)> onChunk,
                    std::function<void(bool, const QString &)> onDone) override
    {
        streaming = true;
        aborted = false;
        streamChatCalls++;
        lastPayload = payload;
        if (script) script(payload, onChunk, onDone);
    }

    void complete(const QJsonObject &payload,
                  std::function<void(bool, const QString &)> onDone) override
    {
        completeCalls++;
        lastPayload = payload;
        if (onDone) onDone(true, QStringLiteral("ok"));
    }

    void abort() override
    {
        aborted = true;
        streaming = false;
    }

    bool isStreaming() const override
    {
        return streaming;
    }
};
