#pragma once

#include <QObject>
#include <QHash>
#include <QList>
#include <QString>
#include <QVariantMap>
#include <QVariantList>
#include <QTimer>

/**
 * 山河AI写作 · 多 Agent 编排层
 *
 * 工作流（默认全自动批量生成模式）：
 *   Setting Agent → Outline Agent → Writer Agent → Reviewer Agent
 *
 * 设计要点（对齐项目硬约束）：
 *   - 异步执行：startWorkflow() 立即返回 workflowId，后续步骤通过
 *     QTimer::singleShot 在事件循环里异步触发，避免阻塞 QML 主线程。
 *   - 可中断：每个步骤前检查 WorkflowState::aborted，true 则提前 return
 *     并 emit workflowCompleted(success=false, reason="aborted")。
 *   - Mock 实现：本类内置 mock Agent 逻辑，不发起真实 LLM 调用；
 *     真实 LLM 接入由后续任务在 runXxxAgent 里替换为 ILlmClient 调用。
 *   - 三段防护：Agent 输出本类内部已脱敏（mock 直接返回安全字符串），
 *     真实接入时由 PromptAssembler 包裹 system/banner/closing，由 LeakGuard 复检。
 *   - 重试机制：审校失败后最多 maxRetries 次重新调用 Writer Agent（仅重写当前章）。
 *
 * 与 bridge 的协作：bridge 持有 AgentOrchestrator 实例（聚合关系），
 * 把 Q_INVOKABLE 方法转发到本类，并把 taskStarted/taskCompleted/workflowCompleted
 * 信号透传给 QML。
 */
class AgentOrchestrator : public QObject
{
    Q_OBJECT
public:
    /// Agent 角色枚举（顺序即工作流默认执行顺序）
    enum class AgentRole {
        SettingAgent = 0,   // 设定 Agent：生成/完善世界观和角色设定
        OutlineAgent = 1,   // 大纲 Agent：根据设定生成分卷大纲和章节目录
        WriterAgent = 2,    // 写作 Agent：根据大纲+设定+前文生成正文
        ReviewerAgent = 3   // 审校 Agent：检查一致性+提出修改建议
    };
    Q_ENUM(AgentRole)

    /// 单个 Agent 任务的状态（用于 getWorkflowStatus 返回）
    enum class TaskStatus {
        Pending = 0,
        Running = 1,
        Succeeded = 2,
        Failed = 3,
        Skipped = 4   // 用户配置跳过（如 autoWrite=false）
    };
    Q_ENUM(TaskStatus)

    /// 单个 Agent 任务记录
    struct AgentTask {
        AgentRole role;
        QString title;          // 任务标题（如「Setting · 生成世界观」）
        QString chapterId;      // 关联章节（Writer/Reviewer 用，Setting/Outline 为空）
        TaskStatus status = TaskStatus::Pending;
        QString result;         // Agent 输出（mock 时为安全占位文本）
        QString error;          // 失败原因（status=Failed 时填）
        int attempt = 0;         // 当前重试次数（0 = 首次）
    };

    /// 章节规格（Outline Agent 输出，Writer Agent 输入）
    struct ChapterSpec {
        int index = 0;          // 1-based 章节序号
        QString title;
        QString brief;          // 章节简要剧情（来自大纲）
    };

    /// 工作流配置（由 bridge 从 QML 接收 QVariantMap 后构造）
    struct WorkflowConfig {
        bool autoSetting = true;        // 自动生成设定
        bool autoOutline = true;        // 自动生成大纲
        bool autoWrite = true;          // 自动写正文
        bool autoReview = true;         // 自动审校
        int chaptersPerBatch = 3;       // 每批写几章（mockChapterCount 与之取较小值）
        int maxRetries = 2;             // 审校失败后最多重试次数
        bool stopOnReviewFail = false;  // 审校失败是否停止整个工作流
        // —— mock 测试钩子 ——
        bool mockReviewAlwaysFail = false;  // mock：审校永远失败（测试重试与停止逻辑）
        int mockChapterCount = 6;           // mock：模拟大纲生成 6 章
    };

    /// 工作流运行态（由 startWorkflow 创建，后续步骤读取/更新）
    struct WorkflowState {
        QString novelId;
        WorkflowConfig config;
        QList<AgentTask> tasks;
        QString settingResult;       // Setting Agent 输出
        QString outlineResult;       // Outline Agent 输出（原始文本）
        QList<ChapterSpec> chapters;  // 解析后的章节列表
        bool completed = false;
        bool success = false;
        bool aborted = false;
    };

    explicit AgentOrchestrator(QObject *parent = nullptr);

    /// 启动一次工作流，返回 workflowId（UUID 风格字符串）。
    /// 异步执行：内部用 QTimer::singleShot(0) 触发 executeWorkflow，
    /// 调用者立即收到 workflowId，并可通过 taskStarted/taskCompleted 信号跟踪进度。
    QString startWorkflow(const QString &novelId, const WorkflowConfig &config);

    /// 查询工作流当前所有任务状态（按执行顺序排列）。
    /// 若 workflowId 不存在，返回空列表。
    QList<AgentTask> getWorkflowStatus(const QString &workflowId) const;

    /// 中断工作流：标记 aborted=true，当前正在执行的步骤会执行完后退出，
    /// 后续步骤不再执行。已完成的任务状态保留。
    void abortWorkflow(const QString &workflowId);

    /// 计算工作流进度百分比（0-100）。
    /// 进度 = 已结束（Succeeded/Failed/Skipped）的任务数 / 总任务数。
    /// workflowId 不存在时返回 0。
    int getWorkflowProgress(const QString &workflowId) const;

    /// 工作流是否已结束（成功/失败/中断均算结束）。
    bool isWorkflowFinished(const QString &workflowId) const;

    /// 把 AgentTask 列表转为 QVariantList（供 bridge 透传给 QML）。
    static QVariantList tasksToVariantList(const QList<AgentTask> &tasks);

    /// 把 WorkflowConfig 从 QVariantMap 解析（供 bridge 从 QML 接收）。
    static WorkflowConfig configFromMap(const QVariantMap &m);

signals:
    /// 某个 Agent 任务开始执行。
    void taskStarted(const QString &workflowId, const QVariantMap &task);
    /// 某个 Agent 任务执行完成（成功或失败均触发）。
    void taskCompleted(const QString &workflowId, const QVariantMap &task);
    /// 整个工作流完成。success=true 表示全部成功；false 表示中断或失败。
    /// reason 为失败/中断原因（"aborted" / "review_failed" / "unknown" 等）。
    void workflowCompleted(const QString &workflowId, bool success, const QString &reason);

private:
    /// 主工作流执行函数（被 startWorkflow 用 QTimer::singleShot 异步触发）。
    void executeWorkflow(const QString &workflowId);

    /// 各 Agent 的 mock 实现（返回脱敏的安全字符串）。
    /// 真实 LLM 接入时替换为 ILlmClient 调用。
    QString runSettingAgent(const QString &novelId);
    QString runOutlineAgent(const QString &novelId, const QString &settingResult,
                            const WorkflowConfig &config);
    QString runWriterAgent(const QString &novelId, const QString &settingResult,
                           const ChapterSpec &chapter, const QString &previousText,
                           int chapterIndex);
    /// 返回 (passed, issues) 二元组：passed=true 表示通过审校。
    /// mock 默认 passed=true；若 mockReviewAlwaysFail 则 passed=false。
    struct ReviewResult {
        bool passed = true;
        QString issues;       // 审校发现的问题（mock 时为占位文本）
    };
    ReviewResult runReviewerAgent(const QString &novelId, const QString &chapterText,
                                  const ChapterSpec &chapter, const WorkflowConfig &config);

    /// 把 outlineResult 解析成 ChapterSpec 列表（mock 实现：按配置生成 N 章）。
    QList<ChapterSpec> parseOutlineChapters(const QString &outlineResult,
                                            const WorkflowConfig &config);

    /// 工具：把单个 AgentTask 转为 QVariantMap（信号透传用）。
    static QVariantMap taskToMap(const AgentTask &t);

    /// 工具：生成 workflowId（时间戳 + 计数器，足够测试唯一性）。
    /// 非 const：会自增 m_idCounter。
    QString generateWorkflowId();

    /// 工具：根据 role 返回中文角色名（供 title 构造）。
    static QString roleLabel(AgentRole r);

    /// 工具：根据 TaskStatus 返回字符串表示（QVariantMap 里用）。
    static QString statusString(TaskStatus s);

private:
    QHash<QString, WorkflowState> m_states;
    int m_idCounter = 0;   // workflowId 计数器
};
