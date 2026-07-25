#pragma once

#include "illmclient.h"

#include <QJsonObject>

/**
 * Unit-test double: scripted streaming for verifying ShanHeBridge orchestration
 * without real network or keys.
 *
 * Stage 2 note: generateWithControl/abortJob/isJobStreaming are stubbed because
 * existing bridge tests only exercise streamChat. The mockLlmClient_* tests
 * cover the generateWithControl state machine via MockLlmClient instead.
 */
class FakeLlmClient : public ILlmClient
{
public:
    bool streaming = false;
    bool aborted = false;
    int streamChatCalls = 0;
    int completeCalls = 0;
    QJsonObject lastPayload;

    // Injected script: invoked on streamChat with (payload, onChunk, onDone).
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

    // ---- Stage 2 stubs (not exercised by existing bridge tests) ----
    void generateWithControl(const GenConfig &,
                             const GenCallbacks &,
                             const QString &) override
    {
        // No-op stub: bridge tests use streamChat, not generateWithControl.
    }

    void abortJob(const QString &) override {}
    bool isJobStreaming(const QString &) const override { return false; }
};
