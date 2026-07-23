#pragma once

#include <functional>
#include <QString>
#include <QJsonObject>

/**
 * LLM 客户端抽象接口（依赖倒置 / Dependency Inversion）。
 *
 * ShanHeBridge 只依赖这个接口，运行时注入具体实现：
 *   - HttpLlmClient : 真实 OpenAI 兼容 /chat/completions 流式调用
 *   - MockLlmClient : 无密钥时的内置演示流式
 *   - FakeLlmClient : 单元测试替身（见 tests/fakellmclient.h）
 *
 * 这样 bridge 不再直接持有 QNetworkAccessManager，首次具备独立单测能力。
 * 使用 std::function 回调而非 Qt 信号，使接口保持纯 C++、可轻松 mock。
 */
class ILlmClient
{
public:
    virtual ~ILlmClient() = default;

    /// 流式对话：每收到一段正文调用 onChunk；结束时调用 onDone(ok, error)。
    virtual void streamChat(const QJsonObject &payload,
                            std::function<void(const QString &)> onChunk,
                            std::function<void(bool, const QString &)> onDone) = 0;

    /// 一次性请求（连通性测试）：结束时调用 onDone(ok, message)。
    virtual void complete(const QJsonObject &payload,
                          std::function<void(bool, const QString &)> onDone) = 0;

    /// 中断当前流式生成。
    virtual void abort() = 0;

    /// 是否正在流式生成中。
    virtual bool isStreaming() const = 0;
};
