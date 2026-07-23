#include "mockllmclient.h"
#include "personas.h"

#include <QJsonArray>

namespace {
constexpr int MOCK_STEP_MS = 26;   // 流式打字节奏
constexpr int MOCK_CHUNK = 4;      // 每次追加字符数

// 从 system prompt 反推当前人格（mock 仅能从消息体识别；与 Personas 单一真相源对齐，
// 不再散落「思考者/奇想版/氛围版」字面量）
QString personaFromSystem(const QString &sys)
{
    for (const QString &k : Personas::keys())
        if (sys.contains(k))
            return k;
    return Personas::keys().isEmpty() ? QString() : Personas::keys().first();
}
}

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

    // 人格 key 列表来自单一真相源（顺序：思考者/奇想版/氛围版）
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
    m_full = buildScriptedText(payload);
    m_pos = 0;
    m_onChunk = onChunk;
    m_onDone = onDone;
    m_timer->start(MOCK_STEP_MS);
}

void MockLlmClient::tick()
{
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
    m_timer->stop();
}

bool MockLlmClient::isStreaming() const
{
    return m_timer->isActive();
}
