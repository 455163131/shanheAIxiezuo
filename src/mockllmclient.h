#pragma once

#include "illmclient.h"

#include <QObject>
#include <QTimer>
#include <QString>
#include <QHash>

/**
 * Built-in demo client: simulates streaming when no API key is configured or
 * backend=="mock". Stage 2 adds generateWithControl so the state-machine
 * callback contract can be exercised without a network.
 */
class MockLlmClient : public QObject, public ILlmClient
{
    Q_OBJECT
public:
    explicit MockLlmClient(QObject *parent = nullptr);

    // ---- Legacy interface ----
    void streamChat(const QJsonObject &payload,
                    std::function<void(const QString &)> onChunk,
                    std::function<void(bool, const QString &)> onDone) override;

    void complete(const QJsonObject &payload,
                  std::function<void(bool, const QString &)> onDone) override;

    void abort() override;
    bool isStreaming() const override;

    // ---- Stage 2: generateWithControl ----
    void generateWithControl(const GenConfig &cfg, const GenCallbacks &cb,
                             const QString &jobId = QString()) override;
    void abortJob(const QString &jobId) override;
    bool isJobStreaming(const QString &jobId) const override;

private slots:
    void tick();

private:
    // generateWithControl per-job state (declared before tickGen so the
    // member function signature resolves to this nested type, not an
    // unrelated forward-declared global struct).
    struct MockJobState {
        QString jobId;
        GenCallbacks callbacks;
        GenConfig config;
        QString full;
        QTimer *genTimer = nullptr;
        int pos = 0;
        bool cancelled = false;
    };

    QString buildScriptedText(const QJsonObject &payload) const;
    QString buildGenScriptedText(const GenConfig &cfg) const;
    void tickGen(MockJobState *job);

    QTimer *m_timer = nullptr;
    QString m_full;
    int m_pos = 0;
    bool m_aborted = false;
    std::function<void(const QString &)> m_onChunk;
    std::function<void(bool, const QString &)> m_onDone;

    QHash<QString, MockJobState *> m_genJobs;
};
