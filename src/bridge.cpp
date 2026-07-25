#include "bridge.h"

#include "httpllmclient.h"
#include "mockllmclient.h"
#include "projectstore.h"
#include "personas.h"
#include "windowscredentialstore.h"
#include "consistencychecker.h"

#include <QDebug>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QSettings>
#include <QMetaType>
#include <QVariant>

namespace {
// 密钥存储策略：
// - Windows（系统凭据库可用）：用 DPAPI 加密，注册表只存 "dpapi:" + base64(密文 blob)。
//   不再是明文、也不再是弱混淆——密钥随用户登录凭据加密，等同 Windows 凭据管理器机制。
// - 其他平台 / DPAPI 不可用：回退到 XOR+Base64 混淆（obfuscation，仅防随手翻看，非加密）。
//   本程序为 Windows 原生桌面端，生产路径走 DPAPI；混淆仅作跨平台兜底。
QByteArray obfuscateKey(const QString &plain) {
    QByteArray b = plain.toUtf8();
    for (int i = 0; i < b.size(); ++i) b[i] = b[i] ^ 0x5A;
    return b.toBase64();
}
QString deobfuscateKey(const QByteArray &b64) {
    QByteArray b = QByteArray::fromBase64(b64);
    for (int i = 0; i < b.size(); ++i) b[i] = b[i] ^ 0x5A;
    return QString::fromUtf8(b);
}

// 把明文编码为要写入注册表/配置的值（优先 DPAPI，否则回退混淆）
QString encodeApiKey(const QString &plain) {
    if (WindowsCredentialStore::available()) {
        QByteArray blob;
        if (WindowsCredentialStore::protect(plain, blob))
            return QStringLiteral("dpapi:") + QString::fromLatin1(blob.toBase64());
    }
    return QString::fromLatin1(obfuscateKey(plain));
}


}

ShanHeBridge::ShanHeBridge(QObject *parent)
    : QObject(parent)
{
    // 加载编译进资源的流派数据
    QFile f(QStringLiteral(":/content/genres.json"));
    if (f.open(QIODevice::ReadOnly)) {
        const QJsonDocument doc = QJsonDocument::fromJson(f.readAll());
        if (doc.isObject()) {
            m_genres = doc.object().value(QStringLiteral("genres")).toArray();
            Q_EMIT genresChanged();
        }
    } else {
        qWarning() << "无法加载流派数据 :/content/genres.json";
    }

    // 默认持有真实 + 演示两套客户端；运行期按 backend / 配置选择（见 activeClient()）
    m_httpClient = new HttpLlmClient(this);
    m_mockClient = new MockLlmClient(this);

    // 书籍持久化层
    m_store = new ProjectStore(this);

    loadConfig();
}

void ShanHeBridge::checkTlsOnStartup()
{
    if (!HttpLlmClient::checkTlsAvailable()) {
        Q_EMIT tlsMissing();
    }
}

// ---------------- 配置持久化 ----------------
void ShanHeBridge::loadConfig()
{
    QSettings s(QStringLiteral("ShanHe"), QStringLiteral("ShanHeWriter"));
    m_apiBase     = s.value(QStringLiteral("api/base")).toString();
    m_apiKey      = decodeApiKey().value_or(QString());
    m_model       = s.value(QStringLiteral("api/model")).toString();
    m_temperature = s.value(QStringLiteral("api/temperature"), 0.8).toDouble();
    m_backend     = s.value(QStringLiteral("api/backend"),
                            QStringLiteral("mock")).toString();

    if (m_httpClient) m_httpClient->configure(m_apiBase, m_apiKey);

    Q_EMIT configChanged();
}

void ShanHeBridge::saveConfig(const QString &base, const QString &key,
                              const QString &model, double temp,
                              const QString &backend)
{
    m_apiBase     = base.trimmed();
    m_apiKey      = key.trimmed();
    m_model       = model.trimmed();
    m_backend     = backend;
    m_temperature = temp;   // 修复：初版此处漏赋值，改温度需重启才生效

    QSettings s(QStringLiteral("ShanHe"), QStringLiteral("ShanHeWriter"));
    s.setValue(QStringLiteral("api/base"), m_apiBase);
    s.setValue(QStringLiteral("api/key"), encodeApiKey(m_apiKey));
    s.setValue(QStringLiteral("api/model"), m_model);
    s.setValue(QStringLiteral("api/temperature"), m_temperature);
    s.setValue(QStringLiteral("api/backend"), m_backend);
    s.sync();

    if (m_httpClient) m_httpClient->configure(m_apiBase, m_apiKey);

    Q_EMIT configChanged();
}

// ---------------- API Key 解码 ----------------
// Bug-3 修复：返回 std::optional<QString> 区分「未配置」与「解密失败」。
// - 未配置（注册表无 api/key 或为空）-> std::nullopt（不 emit error）
// - 解密失败（DPAPI 密文损坏 / 用户切换 / crypt32 不可用）-> std::nullopt + emit error
// - 成功 -> 返回明文
// 调用方拿到 nullopt 后可结合 error 信号判断是配置缺失还是密文损坏，
// 不再像旧版那样把解密失败误报为「未配置」。
std::optional<QString> ShanHeBridge::decodeApiKey() const
{
    QSettings s(QStringLiteral("ShanHe"), QStringLiteral("ShanHeWriter"));
    const QString stored = s.value(QStringLiteral("api/key")).toString();
    if (stored.isEmpty())
        return std::nullopt;

    // 优先 DPAPI 路径：stored 形如 "dpapi:" + base64(blob)
    if (stored.startsWith(QStringLiteral("dpapi:"))) {
        const QByteArray blob = QByteArray::fromBase64(stored.mid(6).toUtf8());
        if (blob.isEmpty()) {
            Q_EMIT const_cast<ShanHeBridge*>(this)->error(
                QStringLiteral("API Key 解密失败：密文格式损坏，请重新配置"));
            return std::nullopt;
        }
        QString plain;
        if (!WindowsCredentialStore::unprotect(blob, plain)) {
            Q_EMIT const_cast<ShanHeBridge*>(this)->error(
                QStringLiteral("API Key 解密失败：DPAPI 解密返回错误，"
                               "可能是用户切换或密文损坏，请重新输入 API Key"));
            return std::nullopt;
        }
        return plain;
    }

    // 旧版混淆格式（兼容早期安装）：XOR+Base64，无解密失败概念，直接反混淆
    return deobfuscateKey(stored.toUtf8());
}

void ShanHeBridge::injectCorruptedApiKeyForTest()
{
    // 写入带 "dpapi:" 前缀的损坏密文：base64 解码后是普通字符串，
    // 不是合法 DPAPI blob，unprotect 必然失败 -> 触发 decodeApiKey 的解密失败路径
    QSettings s(QStringLiteral("ShanHe"), QStringLiteral("ShanHeWriter"));
    s.setValue(QStringLiteral("api/key"),
               QStringLiteral("dpapi:") +
               QString::fromLatin1(QByteArray("not-a-valid-dpapi-blob").toBase64()));
    s.sync();
}
// ---------------- 流派数据 ----------------
QVariantList ShanHeBridge::genres() const
{
    return m_genres.toVariantList();
}

QStringList ShanHeBridge::genreGroups() const
{
    QStringList groups;
    for (const QJsonValue &v : m_genres) {
        const QString g = v.toObject().value(QStringLiteral("group")).toString();
        if (!groups.contains(g))
            groups.append(g);
    }
    return groups;
}

QVariantMap ShanHeBridge::genreById(const QString &id) const
{
    for (const QJsonValue &v : m_genres) {
        const QJsonObject o = v.toObject();
        if (o.value(QStringLiteral("id")).toString() == id)
            return o.toVariantMap();
    }
    return {};
}

QString ShanHeBridge::reduceAIPrompt(const QString &text) const
{
    return text + QStringLiteral(
        "\n\n【降AI痕迹】请用语义重构降低AI味：替换高频套话与排比，增加具象感官细节，"
        "让句式长短错落、偶尔破碎，模仿人类起草时的犹豫与口语感。");
}

QString ShanHeBridge::buildSystemPrompt(const QString &persona, bool reduceAI) const
{
    QString sys = QStringLiteral(
        "你是「山河AI写作」的中文网络小说创作引擎。请严格依据用户给定的流派范式、"
        "风格基调与钩子要求，输出可直接入正文的章节内容。只输出正文，不要解释、不要标题标签。");

    // 人格指令统一来自单一真相源（personas.h），不再散落魔法字符串比较
    const QString p = Personas::systemPrompt(persona);
    if (!p.isEmpty())
        sys += p;

    if (reduceAI)
        sys += QStringLiteral(
            "\n【降AI痕迹】用语义重构降低AI味：替换高频套话与排比，增加具象感官细节，"
            "句式长短错落、偶尔破碎，贴近人类起草口吻。");
    return sys;
}

QStringList ShanHeBridge::personas() const
{
    return Personas::keys();
}

QColor ShanHeBridge::personaColor(const QString &key) const
{
    return Personas::color(key);
}

// ---------------- LLM 客户端选择（依赖注入）----------------
ILlmClient *ShanHeBridge::activeClient() const
{
    if (m_override)
        return m_override;
    const bool useApi = (m_backend == QStringLiteral("api")) && configured();
    return useApi ? static_cast<ILlmClient *>(m_httpClient)
                  : static_cast<ILlmClient *>(m_mockClient);
}

void ShanHeBridge::setLlmClient(ILlmClient *client)
{
    m_override = client;
}

// ---------------- 生成路由 ----------------
void ShanHeBridge::generate(bool reduceAI, const QString &persona,
                            const QString &promptText)
{
    Q_EMIT generationStarted();
    m_cancelled = false;
    m_full.clear();

    QJsonArray messages;
    {
        QJsonObject sysMsg;
        sysMsg["role"] = QStringLiteral("system");
        sysMsg["content"] = buildSystemPrompt(persona, reduceAI);
        messages.append(sysMsg);

        QJsonObject userMsg;
        userMsg["role"] = QStringLiteral("user");
        userMsg["content"] = promptText;
        messages.append(userMsg);
    }

    QJsonObject payload;
    payload["model"] = m_model;
    payload["messages"] = messages;
    payload["temperature"] = m_temperature;
    payload["stream"] = true;

    ILlmClient *client = activeClient();
    if (client == static_cast<ILlmClient *>(m_mockClient))
        Q_EMIT generationProgress(0, QStringLiteral("构思中"));
    else
        Q_EMIT generationProgress(2, QStringLiteral("连接中"));

    client->streamChat(payload,
        [this](const QString &delta) {
            if (delta.isEmpty()) return;
            m_full += delta;
            Q_EMIT generationChunk(delta);
            const int wordMax = m_currentWordCountMax;
            // Bug-5: progress based on actual chars vs word-count cap (1 CN char ~= 2 chars).
            // Capped at 99% (unfinished generation must not reach 100%); 0 when cap unknown.
            const int pct = wordMax > 0 ? qMin(99, m_full.length() * 100 / (wordMax * 2)) : 0;
            Q_EMIT generationProgress(pct, QStringLiteral("生成中"));
        },
        [this](bool ok, const QString &err) {
            // P0-1：用户点「停止」后 m_cancelled 已置位，此处直接返回，
            // 不再误 emit generationDone / error，避免 UI 显示「完成 + 全文」。
            if (m_cancelled) return;
            if (!ok) {
                Q_EMIT error(err);
                return;
            }
            Q_EMIT generationProgress(100, QStringLiteral("完成"));
            Q_EMIT generationDone(m_full);
        });
}

void ShanHeBridge::stopGeneration()
{
    m_cancelled = true;
    // 必须把依赖注入的替身也通知到：测试里 bridge 经 setLlmClient() 把 m_override
    // 设成 FakeLlmClient，不通知它就不会把 fake.aborted 置位，进而让 bridge_cancelSuppressesDone
    // 单测翻车。顺序无关（每个客户端 abort() 都幂等 / 线程安全）。
    if (m_override && m_override != m_httpClient && m_override != m_mockClient)
        m_override->abort();
    if (m_httpClient) m_httpClient->abort();
    if (m_mockClient) m_mockClient->abort();
}

// ---------------- 测试连通性 ----------------
void ShanHeBridge::testConnection()
{
    if (!configured()) {
        Q_EMIT testResult(false, QStringLiteral("请先填写 API 地址、密钥和模型名"));
        return;
    }

    QJsonArray messages;
    QJsonObject userMsg;
    userMsg["role"] = QStringLiteral("user");
    userMsg["content"] = QStringLiteral("ping");
    messages.append(userMsg);

    QJsonObject payload;
    payload["model"] = m_model;
    payload["messages"] = messages;
    payload["max_tokens"] = 1;
    payload["stream"] = false;

    m_httpClient->complete(payload, [this](bool ok, const QString &msg) {
        Q_EMIT testResult(ok, msg);
    });
}

// ---------------- 书籍持久化 ----------------
QVariantList ShanHeBridge::books() const
{
    return m_store ? m_store->listBooks() : QVariantList();
}

void ShanHeBridge::createBook(const QVariantMap &book)
{
    if (!m_store)
        return;
    const QString id = m_store->createBook(book);
    m_store->setLastBookId(id);
    Q_EMIT booksChanged();
    Q_EMIT bookOpened(m_store->loadBook(id));
}

void ShanHeBridge::openBook(const QString &id)
{
    if (!m_store || !m_store->exists(id))
        return;
    m_store->setLastBookId(id);
    Q_EMIT bookOpened(m_store->loadBook(id));
}

void ShanHeBridge::saveBook(const QVariantMap &book)
{
    if (m_store)
        m_store->saveBook(book);
}

// ---------------- 一致性审校 ----------------
// QML 传入的 QVariantList 元素是 QVariantMap，字段约定与 EntityRef 对齐：
//   {id, name, content, summary?, gender?, type?}
// 上层（Studio.qml）从 mockCharacters/mockTerms/mockKnowledge/mockOutlines 构造。
// 这里把 QVariantList 转 QVector<EntityRef>，调 ConsistencyChecker::checkAll，
// 再把 ConsistencyIssue 列表回填成 QVariantMap 给 QML。
QVariantList ShanHeBridge::checkConsistency(const QString &chapterText,
                                            const QString &chapterId,
                                            const QString &previousText,
                                            const QVariantList &characters,
                                            const QVariantList &terms,
                                            const QVariantList &knowledge,
                                            const QVariantList &outlines)
{
    ConsistencyInput input;
    input.chapterText  = chapterText;
    input.chapterId    = chapterId;
    input.previousText = previousText;

    // 把 QVariantList 转 QVector<EntityRef>；缺字段安全降级（空字符串）
    auto toEntities = [](const QVariantList &src, const QString &defaultType) {
        QVector<EntityRef> out;
        out.reserve(src.size());
        for (const QVariant &v : src) {
            const QVariantMap m = v.toMap();
            EntityRef e;
            e.id      = m.value(QStringLiteral("id")).toString();
            e.name    = m.value(QStringLiteral("name")).toString();
            e.content = m.value(QStringLiteral("content")).toString();
            e.summary = m.value(QStringLiteral("summary")).toString();
            e.gender  = m.value(QStringLiteral("gender")).toString();
            e.type    = m.value(QStringLiteral("type")).toString();
            if (e.type.isEmpty())
                e.type = defaultType;
            out.append(e);
        }
        return out;
    };

    input.characters = toEntities(characters, QStringLiteral("character"));
    input.terms      = toEntities(terms,      QStringLiteral("term"));
    input.knowledge  = toEntities(knowledge,  QStringLiteral("knowledge"));
    input.outlines   = toEntities(outlines,   QStringLiteral("outline"));

    ConsistencyChecker checker;
    const QList<ConsistencyIssue> issues = checker.checkAll(input);

    QVariantList result;
    result.reserve(issues.size());
    for (const ConsistencyIssue &issue : issues) {
        QVariantMap m;
        m[QStringLiteral("type")]       = issue.type;
        m[QStringLiteral("severity")]   = issue.severity;
        m[QStringLiteral("title")]      = issue.title;
        m[QStringLiteral("detail")]     = issue.detail;
        m[QStringLiteral("suggestion")] = issue.suggestion;
        m[QStringLiteral("location")]   = issue.location;
        m[QStringLiteral("evidence")]   = issue.evidence;
        result.append(m);
    }
    return result;
}
