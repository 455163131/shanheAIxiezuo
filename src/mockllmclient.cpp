#include "mockllmclient.h"
#include "personas.h"
#include "textutils.h"

#include <QJsonArray>
#include <QDateTime>

namespace {
constexpr int MOCK_STEP_MS = 26;   // legacy streamChat typing rhythm
constexpr int MOCK_CHUNK = 4;      // legacy streamChat chunk size
constexpr int GEN_STEP_MS = 50;    // generateWithControl tick interval
constexpr int GEN_CHUNK = 5;       // generateWithControl chunk size

// Reverse-derive persona from system prompt (mock-only; Personas is the
// single source of truth, no scattered "思考者/奇想版/氛围版" literals).
QString personaFromSystem(const QString &sys)
{
    for (const QString &k : Personas::keys())
        if (sys.contains(k))
            return k;
    return Personas::keys().isEmpty() ? QString() : Personas::keys().first();
}
} // namespace

MockLlmClient::MockLlmClient(QObject *parent)
    : QObject(parent)
{
    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, &MockLlmClient::tick);
}

QString MockLlmClient::buildScriptedText(const QJsonObject &payload) const
{
    QString sys;
    QString user;
    const QJsonArray msgs = payload.value(QStringLiteral("messages")).toArray();
    for (const QJsonValue &v : msgs) {
        const QJsonObject o = v.toObject();
        const QString role = o.value(QStringLiteral("role")).toString();
        if (role == QStringLiteral("system"))
            sys = o.value(QStringLiteral("content")).toString();
        else if (role == QStringLiteral("user"))
            user = o.value(QStringLiteral("content")).toString();
    }

    const QString persona = personaFromSystem(sys);
    const bool reduceAI = sys.contains(QStringLiteral("降AI痕迹"));

    const QStringList ks = Personas::keys();
    const QString thinker = ks.value(0);
    const QString whimsy  = ks.value(1);
    const QString atmosph = ks.value(2);

    QString base = user;
    if (reduceAI)
        base += QStringLiteral(
            "\n\n【降AI痕迹】请用语义重构降低AI味：替换高频套话与排比，增加具象感官细节，"
            "让句式长短错落、偶尔破碎，模仿人类起草时的犹豫与口语感。");

    const QString flavor =
        (persona == whimsy)
            ? QStringLiteral("谁也没料到，转机竟藏在一处最不起眼的角落。")
            : (persona == atmosph)
                  ? QStringLiteral("暮色漫过窗棂，空气里有细微的、说不清的颤动。")
                  : QStringLiteral("他停下脚步，像是听见了某种只有自己懂的回响。");

    return QStringLiteral(
        "【山河·示例生成（未配置 API，当前为内置演示）】\n\n")
        + flavor + QStringLiteral("\n\n") + base.left(120) + QStringLiteral("\n\n")
        + QStringLiteral("风从檐下穿过，带来远处人声与更远的寂静。这一章的伏笔，"
                         "要在三章之后才被轻轻揭开——而此刻，没有人知道。\n\n")
        + QStringLiteral("（进度条与打字机效果均为真实动效；点击右上「设置」填入你的 "
                         "LLM API 后，此处即为真实正文章节。）");
}

void MockLlmClient::streamChat(const QJsonObject &payload,
                               std::function<void(const QString &)> onChunk,
                               std::function<void(bool, const QString &)> onDone)
{
    m_aborted = false;
    m_full = buildScriptedText(payload);
    m_pos = 0;
    m_onChunk = onChunk;
    m_onDone = onDone;
    m_timer->start(MOCK_STEP_MS);
}

void MockLlmClient::tick()
{
    if (m_aborted) {
        m_timer->stop();
        return;
    }
    if (m_pos >= m_full.size()) {
        m_timer->stop();
        if (m_onDone) m_onDone(true, QString());
        return;
    }
    const int n = qMin(MOCK_CHUNK, m_full.size() - m_pos);
    const QString chunk = m_full.mid(m_pos, n);
    m_pos += n;
    if (m_onChunk) m_onChunk(chunk);
    if (m_pos >= m_full.size()) {
        m_timer->stop();
        if (m_onDone) m_onDone(true, QString());
    }
}

void MockLlmClient::complete(const QJsonObject &,
                             std::function<void(bool, const QString &)> onDone)
{
    if (onDone) onDone(true, QStringLiteral("连接成功（演示模式，无需 API）"));
}

void MockLlmClient::abort()
{
    m_aborted = true;
    if (m_timer) m_timer->stop();
    if (m_onDone) {
        auto cb = std::move(m_onDone);
        m_onChunk = nullptr;
        m_onDone = nullptr;
        cb(false, QStringLiteral("aborted"));
    }
}

bool MockLlmClient::isStreaming() const
{
    return m_timer->isActive();
}

// ===========================================================================
// Stage 2: generateWithControl mock state machine
// ===========================================================================

void MockLlmClient::generateWithControl(const GenConfig &cfg, const GenCallbacks &cb,
                                        const QString &jobId)
{
    QString id = jobId.isEmpty()
                     ? QStringLiteral("mock_single_%1").arg(QDateTime::currentMSecsSinceEpoch())
                     : jobId;

    if (m_genJobs.contains(id)) {
        abortJob(id);
    }

    auto *job = new MockJobState();
    job->jobId = id;
    job->callbacks = cb;
    job->config = cfg;
    job->full = buildGenScriptedText(cfg);
    m_genJobs.insert(id, job);

    if (cb.onMeta) cb.onMeta(QStringLiteral("stream_wait"));

    job->genTimer = new QTimer(this);
    job->genTimer->setSingleShot(false);
    connect(job->genTimer, &QTimer::timeout, this, [this, job]() {
        tickGen(job);
    });
    job->genTimer->start(GEN_STEP_MS);
}

void MockLlmClient::tickGen(MockJobState *job)
{
    if (job->cancelled) {
        job->genTimer->stop();
        return;
    }
    if (job->pos >= job->full.size()) {
        job->genTimer->stop();
        GenResult result;
        result.finishReason = QStringLiteral("stop");
        result.wordCount = TextUtils::countWords(job->full);
        result.continueRounds = 0;
        if (job->callbacks.onMeta)
            job->callbacks.onMeta(QStringLiteral("stream_done"));
        if (job->callbacks.onDone)
            job->callbacks.onDone(true, QString(), job->full, result);
        m_genJobs.remove(job->jobId);
        delete job;
        return;
    }
    const int step = qMin(GEN_CHUNK, job->full.size() - job->pos);
    const QString delta = job->full.mid(job->pos, step);
    job->pos += step;
    if (job->callbacks.onDelta) job->callbacks.onDelta(delta);
}

void MockLlmClient::abortJob(const QString &jobId)
{
    auto it = m_genJobs.find(jobId);
    if (it == m_genJobs.end()) return;
    MockJobState *job = it.value();
    if (job->cancelled) return;  // idempotent
    job->cancelled = true;
    if (job->genTimer) job->genTimer->stop();

    GenResult result;
    result.finishReason = QStringLiteral("aborted");
    result.wordCount = TextUtils::countWords(job->full);
    if (job->callbacks.onDone)
        job->callbacks.onDone(false, QStringLiteral("aborted"), job->full, result);

    m_genJobs.remove(jobId);
    delete job;
}

bool MockLlmClient::isJobStreaming(const QString &jobId) const
{
    return m_genJobs.contains(jobId);
}

QString MockLlmClient::buildGenScriptedText(const GenConfig &cfg) const
{
    Q_UNUSED(cfg)
    // Scripted mock text (~70 chars). Production would derive flavor from
    // persona; here we keep a single stable script so the basic-round test
    // has deterministic output.
    return QStringLiteral(
        "林凡踏入虚空裂缝，眼前光芒大盛。他感受到一股浩瀚的力量涌入体内，"
        "经脉中的灵力开始疯狂运转。这是突破的征兆——筑基期，即将大成。"
        "远处的山峦在晨曦中若隐若现，仿佛在为这一刻作证。"
    );
}
