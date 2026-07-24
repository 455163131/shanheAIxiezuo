#pragma once

#include <QObject>
#include <QColor>
#include <optional>
#include <QJsonArray>
#include <QJsonObject>
#include <QString>
#include <QStringList>
#include <QVariantList>
#include <QVariantMap>

#include "illmclient.h"

class HttpLlmClient;
class MockLlmClient;
class ProjectStore;

/**
 * 山河AI写作 · C++ 内核桥接（QML 暴露的全局对象 "ShanHe"）
 *
 * 职责收敛（相对初版的「上帝类」整改）：
 *  - 流派数据加载与提示词编排（领域逻辑）
 *  - 配置持久化（QSettings）
 *  - 作为编排者，把 LLM 调用委派给注入的 ILlmClient（真实 / 演示 / 测试替身）
 *
 * 网络、SSE 解析、mock 流式等传输细节已下沉到 ILlmClient 的实现中，
 * 因此本类首次具备独立单测能力。QML 暴露的接口（属性 / Q_INVOKABLE）
 * 与初版完全一致，QML 层无需任何改动。
 */
class ShanHeBridge : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVariantList genres READ genres NOTIFY genresChanged)
    Q_PROPERTY(QStringList genreGroups READ genreGroups NOTIFY genresChanged)
    Q_PROPERTY(QString apiBase READ apiBase NOTIFY configChanged)
    Q_PROPERTY(QString apiKey READ apiKey NOTIFY configChanged)
    Q_PROPERTY(QString model READ model NOTIFY configChanged)
    Q_PROPERTY(double temperature READ temperature NOTIFY configChanged)
    Q_PROPERTY(QString backend READ backend NOTIFY configChanged)
    Q_PROPERTY(bool configured READ configured NOTIFY configChanged)
    Q_PROPERTY(QVariantList books READ books NOTIFY booksChanged)

public:
    explicit ShanHeBridge(QObject *parent = nullptr);

    QVariantList genres() const;
    QStringList genreGroups() const;

    /// 书架（持久化的书籍列表），供 QML 绑定
    QVariantList books() const;

    /// 持久化：创建 / 打开 / 保存一本书（book 为完整书籍对象，含 id 与 chapters）
    Q_INVOKABLE void createBook(const QVariantMap &book);
    Q_INVOKABLE void openBook(const QString &id);
    Q_INVOKABLE void saveBook(const QVariantMap &book);

    QString apiBase() const { return m_apiBase; }
    QString apiKey() const { return m_apiKey; }
    QString model() const { return m_model; }
    double temperature() const { return m_temperature; }
    QString backend() const { return m_backend; }
    bool configured() const {
        return !m_apiBase.isEmpty() && !m_apiKey.isEmpty() && !m_model.isEmpty();
    }

    Q_INVOKABLE QVariantMap genreById(const QString &id) const;
    Q_INVOKABLE QString reduceAIPrompt(const QString &text) const;

    /// 人格（路由）单一真相源：列表与主题色，供 QML 绑定，杜绝魔法字符串
    Q_INVOKABLE QStringList personas() const;
    Q_INVOKABLE QColor personaColor(const QString &key) const;

    /// 保存配置并持久化到本机（QSettings）。backend: "api" | "mock"
    Q_INVOKABLE void saveConfig(const QString &base, const QString &key,
                                const QString &model, double temp,
                                const QString &backend);

    /// 测试连通性：结果通过 testResult 信号返回
    Q_INVOKABLE void testConnection();

    /// 触发一次章节生成（按当前配置路由到真实 API 或 mock）
    Q_INVOKABLE void generate(bool reduceAI, const QString &persona,
                              const QString &promptText);

    /// 中断当前生成
    Q_INVOKABLE void stopGeneration();

    /// 依赖注入入口：测试或外部装配时注入 ILlmClient 替身（bridge 不拥有其生命周期）
    void setLlmClient(ILlmClient *client);

    /// 启动时检测 TLS 插件可用性（缺失则 emit tlsMissing 让 QML 设置页标红）
    void checkTlsOnStartup();

    /// 解码持久化的 API Key。
    /// - 未配置（注册表无 api/key）-> std::nullopt
    /// - 解密失败（DPAPI 密文损坏 / 用户切换）-> std::nullopt 并 emit error（含「解密失败」）
    /// - 成功 -> 返回明文
    /// 与「未配置」可区分：调用方拿到 nullopt 后可结合 error 信号判断是配置缺失还是密文损坏。
    std::optional<QString> decodeApiKey() const;

    /// [仅测试] 把一个损坏的「dpapi:」前缀密文写入注册表，用于触发解密失败路径。
    void injectCorruptedApiKeyForTest();

signals:
    void genresChanged();
    void configChanged();
    void generationStarted();
    void generationProgress(int pct, QString stage);
    void generationChunk(QString text);
    void generationDone(QString fullText);
    void error(QString msg);
    void testResult(bool ok, QString msg);

    /// TLS/HTTPS 自检未通过（Task 3 预留，本任务不实装）
    void tlsMissing();

    /// 书架变化（新建 / 删除）
    void booksChanged();
    /// 一本书已就绪（创建或打开后），带完整书籍对象，供 QML 进入创作台
    void bookOpened(const QVariantMap &book);

private:
    QJsonArray m_genres;

    // 配置
    QString m_apiBase;
    QString m_apiKey;
    QString m_model;
    QString m_backend = QStringLiteral("mock");
    double  m_temperature = 0.8;

    // 生成状态（由桥接层累积，供进度 / 完成信号使用）
    QString m_full;
    bool m_cancelled = false;

    // LLM 客户端（依赖注入）：默认由 bridge 持有真实 / 演示实现；
    // 测试可经 setLlmClient() 覆盖为替身。
    ILlmClient *m_override = nullptr;
    HttpLlmClient *m_httpClient = nullptr;
    MockLlmClient *m_mockClient = nullptr;

    // 书籍持久化层（P2：解决「重启即丢」）
    ProjectStore *m_store = nullptr;

    void loadConfig();
    QString buildSystemPrompt(const QString &persona, bool reduceAI) const;
    ILlmClient *activeClient() const;
};
