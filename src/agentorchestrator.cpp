#include "agentorchestrator.h"

#include <QDateTime>
#include <QCoreApplication>
#include <QTimer>
#include <QRandomGenerator>

/**
 * 实现说明（与头文件 docstring 对齐）：
 *  - executeWorkflow() 为同步串行执行（在 QML 主线程事件循环内分段进行，
 *    通过 QTimer::singleShot(0) 异步触发，避免阻塞 startWorkflow 返回 workflowId）。
 *  - 每个 Agent 步骤前后检查 m_states[id].aborted，确保可中断。
 *  - Mock 输出固定模板文本，方便测试断言；真实 LLM 接入只需替换四个 runXxxAgent。
 *  - 章节批处理：取 min(chaptersPerBatch, mockChapterCount) 章逐章写 + 审校。
 */

AgentOrchestrator::AgentOrchestrator(QObject *parent)
    : QObject(parent)
{
}

QString AgentOrchestrator::startWorkflow(const QString &novelId, const WorkflowConfig &config)
{
    const QString id = generateWorkflowId();
    WorkflowState st;
    st.novelId = novelId;
    st.config = config;
    st.completed = false;
    st.success = false;
    st.aborted = false;
    m_states.insert(id, st);

    // 异步触发：让 startWorkflow 立即返回 workflowId，
    // 调用方（bridge/QML）可立即绑定信号再让 workflow 真正开跑。
    QTimer::singleShot(0, this, [this, id]() { this->executeWorkflow(id); });
    return id;
}

QList<AgentOrchestrator::AgentTask> AgentOrchestrator::getWorkflowStatus(const QString &workflowId) const
{
    auto it = m_states.constFind(workflowId);
    if (it == m_states.constEnd()) return {};
    return it->tasks;
}

void AgentOrchestrator::abortWorkflow(const QString &workflowId)
{
    auto it = m_states.find(workflowId);
    if (it == m_states.end()) return;
    it->aborted = true;
}

int AgentOrchestrator::getWorkflowProgress(const QString &workflowId) const
{
    auto it = m_states.constFind(workflowId);
    if (it == m_states.constEnd()) return 0;
    const auto &tasks = it->tasks;
    if (tasks.isEmpty()) return 0;
    int finished = 0;
    for (const auto &t : tasks) {
        if (t.status == TaskStatus::Succeeded
            || t.status == TaskStatus::Failed
            || t.status == TaskStatus::Skipped) {
            ++finished;
        }
    }
    return int(std::lround(double(finished) * 100.0 / double(tasks.size())));
}

bool AgentOrchestrator::isWorkflowFinished(const QString &workflowId) const
{
    auto it = m_states.constFind(workflowId);
    if (it == m_states.constEnd()) return false;
    return it->completed;
}

QVariantList AgentOrchestrator::tasksToVariantList(const QList<AgentTask> &tasks)
{
    QVariantList out;
    out.reserve(tasks.size());
    for (const auto &t : tasks) out.append(taskToMap(t));
    return out;
}

AgentOrchestrator::WorkflowConfig AgentOrchestrator::configFromMap(const QVariantMap &m)
{
    WorkflowConfig c;
    c.autoSetting         = m.value(QStringLiteral("autoSetting"), true).toBool();
    c.autoOutline         = m.value(QStringLiteral("autoOutline"), true).toBool();
    c.autoWrite           = m.value(QStringLiteral("autoWrite"), true).toBool();
    c.autoReview          = m.value(QStringLiteral("autoReview"), true).toBool();
    c.chaptersPerBatch    = m.value(QStringLiteral("chaptersPerBatch"), 3).toInt();
    c.maxRetries          = m.value(QStringLiteral("maxRetries"), 2).toInt();
    c.stopOnReviewFail    = m.value(QStringLiteral("stopOnReviewFail"), false).toBool();
    c.mockReviewAlwaysFail = m.value(QStringLiteral("mockReviewAlwaysFail"), false).toBool();
    c.mockChapterCount    = m.value(QStringLiteral("mockChapterCount"), 6).toInt();
    return c;
}

// ============== 主工作流 ==============

void AgentOrchestrator::executeWorkflow(const QString &workflowId)
{
    auto it = m_states.find(workflowId);
    if (it == m_states.end()) return;
    WorkflowState &st = it.value();

    QString failReason;

    // ===== 步骤 1：Setting Agent =====
    if (st.config.autoSetting) {
        if (st.aborted) { failReason = QStringLiteral("aborted"); goto done; }
        AgentTask t;
        t.role = AgentRole::SettingAgent;
        t.title = QStringLiteral("设定 · 生成世界观与人物卡");
        t.chapterId = QString();
        t.status = TaskStatus::Running;
        st.tasks.append(t);
        emit taskStarted(workflowId, taskToMap(t));

        st.settingResult = runSettingAgent(st.novelId);

        // aborted 可能在 mock 调用期间被设置（虽然 mock 是同步的，留作未来真实异步场景的保险）
        if (st.aborted) {
            st.tasks.last().status = TaskStatus::Failed;
            st.tasks.last().error = QStringLiteral("aborted");
            emit taskCompleted(workflowId, taskToMap(st.tasks.last()));
            failReason = QStringLiteral("aborted");
            goto done;
        }
        st.tasks.last().status = TaskStatus::Succeeded;
        st.tasks.last().result = st.settingResult;
        emit taskCompleted(workflowId, taskToMap(st.tasks.last()));
    } else {
        AgentTask t;
        t.role = AgentRole::SettingAgent;
        t.title = QStringLiteral("设定 · 跳过（用户配置）");
        t.status = TaskStatus::Skipped;
        st.tasks.append(t);
        emit taskStarted(workflowId, taskToMap(t));
        emit taskCompleted(workflowId, taskToMap(t));
    }

    // ===== 步骤 2：Outline Agent =====
    if (st.config.autoOutline) {
        if (st.aborted) { failReason = QStringLiteral("aborted"); goto done; }
        AgentTask t;
        t.role = AgentRole::OutlineAgent;
        t.title = QStringLiteral("大纲 · 生成分卷大纲与章节目录");
        t.status = TaskStatus::Running;
        st.tasks.append(t);
        emit taskStarted(workflowId, taskToMap(t));

        st.outlineResult = runOutlineAgent(st.novelId, st.settingResult, st.config);
        st.chapters = parseOutlineChapters(st.outlineResult, st.config);

        if (st.aborted) {
            st.tasks.last().status = TaskStatus::Failed;
            st.tasks.last().error = QStringLiteral("aborted");
            emit taskCompleted(workflowId, taskToMap(st.tasks.last()));
            failReason = QStringLiteral("aborted");
            goto done;
        }
        st.tasks.last().status = TaskStatus::Succeeded;
        st.tasks.last().result = st.outlineResult;
        emit taskCompleted(workflowId, taskToMap(st.tasks.last()));
    } else {
        AgentTask t;
        t.role = AgentRole::OutlineAgent;
        t.title = QStringLiteral("大纲 · 跳过（用户配置）");
        t.status = TaskStatus::Skipped;
        st.tasks.append(t);
        emit taskStarted(workflowId, taskToMap(t));
        emit taskCompleted(workflowId, taskToMap(t));
    }

    // ===== 步骤 3 & 4：Writer + Reviewer（逐章批处理）=====
    if (st.config.autoWrite) {
        const int totalChapters = st.chapters.size();
        const int batch = std::min(totalChapters, st.config.chaptersPerBatch);

        for (int i = 0; i < batch; ++i) {
            if (st.aborted) { failReason = QStringLiteral("aborted"); goto done; }

            const ChapterSpec &ch = st.chapters.at(i);
            const QString chapterId = QStringLiteral("ch%1").arg(ch.index);

            // 前文（取上一章 mock 输出，简化）
            QString previousText;
            if (i > 0) {
                // 从已完成任务里找上一章 Writer 的输出
                for (int k = st.tasks.size() - 1; k >= 0; --k) {
                    if (st.tasks[k].role == AgentRole::WriterAgent
                        && st.tasks[k].chapterId == QStringLiteral("ch%1").arg(st.chapters.at(i - 1).index)) {
                        previousText = st.tasks[k].result;
                        break;
                    }
                }
            }

            // ===== Writer Agent =====
            AgentTask wt;
            wt.role = AgentRole::WriterAgent;
            wt.title = QStringLiteral("写作 · 第 %1 章「%2」").arg(ch.index).arg(ch.title);
            wt.chapterId = chapterId;
            wt.status = TaskStatus::Running;
            st.tasks.append(wt);
            emit taskStarted(workflowId, taskToMap(st.tasks.last()));

            QString chapterText = runWriterAgent(st.novelId, st.settingResult,
                                                 ch, previousText, i);

            if (st.aborted) {
                st.tasks.last().status = TaskStatus::Failed;
                st.tasks.last().error = QStringLiteral("aborted");
                emit taskCompleted(workflowId, taskToMap(st.tasks.last()));
                failReason = QStringLiteral("aborted");
                goto done;
            }

            // ===== Reviewer Agent（与 Writer 配对）=====
            if (st.config.autoReview) {
                // 先发 Writer 完成信号（结果先存）
                st.tasks.last().status = TaskStatus::Succeeded;
                st.tasks.last().result = chapterText;
                emit taskCompleted(workflowId, taskToMap(st.tasks.last()));

                if (st.aborted) { failReason = QStringLiteral("aborted"); goto done; }

                AgentTask rt;
                rt.role = AgentRole::ReviewerAgent;
                rt.title = QStringLiteral("审校 · 第 %1 章「%2」").arg(ch.index).arg(ch.title);
                rt.chapterId = chapterId;
                rt.status = TaskStatus::Running;
                rt.attempt = 0;
                st.tasks.append(rt);
                emit taskStarted(workflowId, taskToMap(st.tasks.last()));

                ReviewResult rr = runReviewerAgent(st.novelId, chapterText, ch, st.config);

                // 重试循环：审校失败 -> 重写 -> 再审
                while (!rr.passed && rt.attempt < st.config.maxRetries) {
                    if (st.aborted) {
                        st.tasks.last().status = TaskStatus::Failed;
                        st.tasks.last().error = QStringLiteral("aborted");
                        st.tasks.last().attempt = rt.attempt;
                        emit taskCompleted(workflowId, taskToMap(st.tasks.last()));
                        failReason = QStringLiteral("aborted");
                        goto done;
                    }
                    rt.attempt++;
                    chapterText = runWriterAgent(st.novelId, st.settingResult,
                                                 ch, previousText, i);
                    rr = runReviewerAgent(st.novelId, chapterText, ch, st.config);
                }

                if (rr.passed) {
                    st.tasks.last().status = TaskStatus::Succeeded;
                    st.tasks.last().result = rr.issues.isEmpty()
                        ? QStringLiteral("审校通过")
                        : rr.issues;
                    st.tasks.last().attempt = rt.attempt;
                    emit taskCompleted(workflowId, taskToMap(st.tasks.last()));
                } else {
                    st.tasks.last().status = TaskStatus::Failed;
                    st.tasks.last().error = QStringLiteral("审校未通过（重试 %1 次后仍失败）").arg(rt.attempt);
                    st.tasks.last().attempt = rt.attempt;
                    emit taskCompleted(workflowId, taskToMap(st.tasks.last()));

                    if (st.config.stopOnReviewFail) {
                        failReason = QStringLiteral("review_failed");
                        goto done;
                    }
                    // 否则继续写下一章
                }
            } else {
                // 不审校，直接完成 Writer
                st.tasks.last().status = TaskStatus::Succeeded;
                st.tasks.last().result = chapterText;
                emit taskCompleted(workflowId, taskToMap(st.tasks.last()));
            }
        }
    } else {
        // autoWrite=false：跳过 Writer+Reviewer
        AgentTask t;
        t.role = AgentRole::WriterAgent;
        t.title = QStringLiteral("写作 · 跳过（用户配置）");
        t.status = TaskStatus::Skipped;
        st.tasks.append(t);
        emit taskStarted(workflowId, taskToMap(t));
        emit taskCompleted(workflowId, taskToMap(t));
    }

    // 全部成功（fall-through 路径）：仅当没有任何任务失败时才标记 success=true。
    // stopOnReviewFail=false 时虽然会继续写下一章，但只要存在 Failed 任务，
    // 工作流整体不算成功（success=false，与头文件 docstring 「success=true 表示全部成功」一致）。
    st.success = true;

done:
    st.completed = true;
    // goto done 路径（abort/review_failed）success 仍为 false，无需再查；
    // fall-through 路径 success=true，需复检是否有 Failed 任务（stopOnReviewFail=false 容错场景）。
    if (st.success) {
        for (const auto &t : st.tasks) {
            if (t.status == TaskStatus::Failed) {
                st.success = false;
                if (failReason.isEmpty()) {
                    failReason = QStringLiteral("partial_failure");
                }
                break;
            }
        }
    }
    emit workflowCompleted(workflowId, st.success, failReason);
}

// ============== Mock Agent 实现 ==============

QString AgentOrchestrator::runSettingAgent(const QString &novelId)
{
    // mock：返回固定模板，避免发起真实 LLM 调用
    Q_UNUSED(novelId)
    return QStringLiteral(
        "【世界观】\n"
        "九州大陆，分为东南西北四域，每域有大国一、小国若干。\n"
        "修真界等级：炼气、筑基、金丹、元婴、化神、渡劫。\n\n"
        "【人物卡】\n"
        "主角：林墨，青云门内门弟子，资质平平但意志坚韧。\n"
        "女主：苏清雪，天音寺圣女，温柔聪慧。\n"
        "反派：叶凌天，魔道天才，野心勃勃。\n\n"
        "【时间线】\n"
        "1. 林墨入门被欺\n"
        "2. 偶得奇遇突破筑基\n"
        "3. 与苏清雪结识\n"
        "4. 与叶凌天初次冲突\n"
        "5. 金丹大战\n"
        "6. 渡劫飞升\n");
}

QString AgentOrchestrator::runOutlineAgent(const QString &novelId,
                                           const QString &settingResult,
                                           const WorkflowConfig &config)
{
    Q_UNUSED(novelId)
    Q_UNUSED(settingResult)
    const int n = std::max(1, config.mockChapterCount);
    QString out = QStringLiteral("全书大纲：\n");
    for (int i = 1; i <= n; ++i) {
        out += QStringLiteral("第 %1 章 章节标题_%1 简要剧情：本章推进主线第 %1 步。\n").arg(i);
    }
    return out;
}

QString AgentOrchestrator::runWriterAgent(const QString &novelId,
                                          const QString &settingResult,
                                          const ChapterSpec &chapter,
                                          const QString &previousText,
                                          int chapterIndex)
{
    Q_UNUSED(novelId)
    Q_UNUSED(settingResult)
    Q_UNUSED(previousText)
    Q_UNUSED(chapterIndex)
    // mock 正文：500 字左右占位
    return QStringLiteral(
        "第 %1 章「%2」\n\n"
        "【正文开头】晨曦微露，青云山脉云雾缭绕。林墨立于悬崖之上，望向远方连绵群山，"
        "心中翻涌着难以名状的情绪。他紧握双拳，回想起入门以来遭受的冷眼与欺辱，"
        "一股不甘之意从丹田升起。\n\n"
        "【情节推进】这一日，他在后山偶然发现一处古洞，洞中藏有一卷泛黄古籍。"
        "翻开书页，一道金光没入眉心，无数玄奥符文在他识海中流转。"
        "原来这是一部失传已久的上古功法——《混元真经》。\n\n"
        "【本章爽点】林墨只觉体内灵力暴涨，竟一举突破炼气期三层，"
        "周围空气都因他而震颤。这一刻，他知道自己的人生将彻底不同。\n\n"
        "【收尾】走出古洞，林墨眼神变得坚定。从此刻起，"
        "他不再是那个任人欺凌的废物弟子，他要让整个青云门为今日的轻视付出代价。\n"
        ).arg(chapter.index).arg(chapter.title);
}

AgentOrchestrator::ReviewResult
AgentOrchestrator::runReviewerAgent(const QString &novelId,
                                    const QString &chapterText,
                                    const ChapterSpec &chapter,
                                    const WorkflowConfig &config)
{
    Q_UNUSED(novelId)
    Q_UNUSED(chapterText)
    ReviewResult r;
    if (config.mockReviewAlwaysFail) {
        r.passed = false;
        r.issues = QStringLiteral("【审校问题】第 %1 章「%2」：检测到 mock 强制失败标记，"
                                   "用于测试重试与停止逻辑。").arg(chapter.index).arg(chapter.title);
    } else {
        r.passed = true;
        r.issues = QStringLiteral("【审校通过】第 %1 章「%2」：人物、设定、剧情一致性均无问题。")
                       .arg(chapter.index).arg(chapter.title);
    }
    return r;
}

QList<AgentOrchestrator::ChapterSpec>
AgentOrchestrator::parseOutlineChapters(const QString &outlineResult,
                                        const WorkflowConfig &config)
{
    Q_UNUSED(outlineResult)
    // mock：直接按 mockChapterCount 生成 ChapterSpec
    const int n = std::max(1, config.mockChapterCount);
    QList<ChapterSpec> out;
    out.reserve(n);
    for (int i = 1; i <= n; ++i) {
        ChapterSpec c;
        c.index = i;
        c.title = QStringLiteral("章节标题_%1").arg(i);
        c.brief = QStringLiteral("简要剧情：本章推进主线第 %1 步。").arg(i);
        out.append(c);
    }
    return out;
}

// ============== 工具函数 ==============

QVariantMap AgentOrchestrator::taskToMap(const AgentTask &t)
{
    QVariantMap m;
    m[QStringLiteral("role")] = static_cast<int>(t.role);
    m[QStringLiteral("roleLabel")] = roleLabel(t.role);
    m[QStringLiteral("title")] = t.title;
    m[QStringLiteral("chapterId")] = t.chapterId;
    m[QStringLiteral("status")] = static_cast<int>(t.status);
    m[QStringLiteral("statusLabel")] = statusString(t.status);
    m[QStringLiteral("result")] = t.result;
    m[QStringLiteral("error")] = t.error;
    m[QStringLiteral("attempt")] = t.attempt;
    return m;
}

QString AgentOrchestrator::generateWorkflowId()
{
    ++m_idCounter;
    return QStringLiteral("wf_%1_%2")
        .arg(QDateTime::currentDateTime().toMSecsSinceEpoch())
        .arg(m_idCounter);
}

QString AgentOrchestrator::roleLabel(AgentRole r)
{
    switch (r) {
    case AgentRole::SettingAgent:   return QStringLiteral("设定");
    case AgentRole::OutlineAgent:   return QStringLiteral("大纲");
    case AgentRole::WriterAgent:    return QStringLiteral("写作");
    case AgentRole::ReviewerAgent:   return QStringLiteral("审校");
    }
    return QStringLiteral("未知");
}

QString AgentOrchestrator::statusString(TaskStatus s)
{
    switch (s) {
    case TaskStatus::Pending:   return QStringLiteral("待执行");
    case TaskStatus::Running:   return QStringLiteral("执行中");
    case TaskStatus::Succeeded: return QStringLiteral("成功");
    case TaskStatus::Failed:    return QStringLiteral("失败");
    case TaskStatus::Skipped:   return QStringLiteral("跳过");
    }
    return QStringLiteral("未知");
}
