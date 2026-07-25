#include <QtTest>
#include "agentorchestrator.h"

#include <QCoreApplication>
#include <QEventLoop>
#include <QTimer>
#include <QVariantMap>
#include <QVariantList>

/**
 * 多 Agent 工作流单测
 *
 * 覆盖：
 *  - startWorkflow 异步返回 workflowId
 *  - 全流程执行成功（设定→大纲→写作×N→审校×N）
 *  - getWorkflowStatus / getWorkflowProgress
 *  - abortWorkflow 中断
 *  - mockReviewAlwaysFail 触发重试与最终失败
 *  - stopOnReviewFail 提前终止
 *  - 跳过 Agent（autoXxx=false → Skipped 任务）
 *  - configFromMap 解析
 *  - tasksToVariantList 序列化
 *
 * 测试采用 QEventLoop + QTimer::singleShot 等待 workflowCompleted 信号，
 * 避免 sleep 死等。
 */
class TestAgentOrchestrator : public QObject
{
    Q_OBJECT
private slots:
    void initTestCase();
    void cleanupTestCase() {}

    void testStartWorkflowReturnsId();
    void testFullWorkflowSucceeds();
    void testWorkflowProgress();
    void testAbortWorkflow();
    void testReviewAlwaysFailRetries();
    void testStopOnReviewFail();
    void testSkipAgents();
    void testConfigFromMap();
    void testTasksToVariantList();

private:
    /// 等待 workflow 完成，最长 timeoutMs 毫秒。返回是否完成（true=completed）。
    bool waitForWorkflow(AgentOrchestrator &o, const QString &id, int timeoutMs = 5000);
};

void TestAgentOrchestrator::initTestCase()
{
    QCoreApplication::processEvents();
}

bool TestAgentOrchestrator::waitForWorkflow(AgentOrchestrator &o, const QString &id, int timeoutMs)
{
    QEventLoop loop;
    QTimer timeout;
    timeout.setSingleShot(true);
    QString capturedId;
    QObject::connect(&o, &AgentOrchestrator::workflowCompleted,
                     &loop, [&](const QString &wid, bool, const QString &) {
        if (wid == id) loop.quit();
    });
    QObject::connect(&timeout, &QTimer::timeout, &loop, &QEventLoop::quit);
    timeout.start(timeoutMs);
    loop.exec();
    return o.isWorkflowFinished(id);
}

void TestAgentOrchestrator::testStartWorkflowReturnsId()
{
    AgentOrchestrator o;
    AgentOrchestrator::WorkflowConfig cfg;
    cfg.mockChapterCount = 2;
    cfg.chaptersPerBatch = 1;
    const QString id = o.startWorkflow(QStringLiteral("novel_test_1"), cfg);
    QVERIFY(!id.isEmpty());
    QVERIFY(id.startsWith(QStringLiteral("wf_")));
    // 等待完成（避免下一个用例污染事件循环）
    QVERIFY(waitForWorkflow(o, id));
    QVERIFY(o.isWorkflowFinished(id));
}

void TestAgentOrchestrator::testFullWorkflowSucceeds()
{
    AgentOrchestrator o;
    AgentOrchestrator::WorkflowConfig cfg;
    cfg.autoSetting = true;
    cfg.autoOutline = true;
    cfg.autoWrite = true;
    cfg.autoReview = true;
    cfg.chaptersPerBatch = 3;
    cfg.mockChapterCount = 3;
    cfg.maxRetries = 2;
    cfg.stopOnReviewFail = false;
    cfg.mockReviewAlwaysFail = false;

    QString capturedId;
    bool capturedSuccess = false;
    QString capturedReason;
    QObject::connect(&o, &AgentOrchestrator::workflowCompleted,
                     this, [&](const QString &wid, bool ok, const QString &reason) {
        capturedId = wid;
        capturedSuccess = ok;
        capturedReason = reason;
    });

    const QString id = o.startWorkflow(QStringLiteral("novel_full_ok"), cfg);
    QVERIFY(waitForWorkflow(o, id));

    QVERIFY(o.isWorkflowFinished(id));
    QVERIFY(capturedSuccess);
    QVERIFY(capturedReason.isEmpty());   // 成功时 reason 为空

    // 任务列表校验：1 setting + 1 outline + 3 × (1 writer + 1 reviewer) = 8
    const auto tasks = o.getWorkflowStatus(id);
    QCOMPARE(tasks.size(), 8);

    // 第一个是 Setting Agent
    QCOMPARE(int(tasks[0].role), int(AgentOrchestrator::AgentRole::SettingAgent));
    QCOMPARE(int(tasks[0].status), int(AgentOrchestrator::TaskStatus::Succeeded));

    // 第二个是 Outline Agent
    QCOMPARE(int(tasks[1].role), int(AgentOrchestrator::AgentRole::OutlineAgent));
    QCOMPARE(int(tasks[1].status), int(AgentOrchestrator::TaskStatus::Succeeded));

    // 第三个是第一个 Writer Agent
    QCOMPARE(int(tasks[2].role), int(AgentOrchestrator::AgentRole::WriterAgent));
    QCOMPARE(int(tasks[2].status), int(AgentOrchestrator::TaskStatus::Succeeded));
    QVERIFY(!tasks[2].chapterId.isEmpty());

    // 第四个是第一个 Reviewer Agent（成功）
    QCOMPARE(int(tasks[3].role), int(AgentOrchestrator::AgentRole::ReviewerAgent));
    QCOMPARE(int(tasks[3].status), int(AgentOrchestrator::TaskStatus::Succeeded));

    // 所有任务都不应该是 Pending（已结束）
    for (const auto &t : tasks) {
        QVERIFY2(t.status != AgentOrchestrator::TaskStatus::Pending,
                 qPrintable(QStringLiteral("Task still pending: ") + t.title));
        QVERIFY2(t.status != AgentOrchestrator::TaskStatus::Running,
                 qPrintable(QStringLiteral("Task still running: ") + t.title));
    }
}

void TestAgentOrchestrator::testWorkflowProgress()
{
    AgentOrchestrator o;
    AgentOrchestrator::WorkflowConfig cfg;
    cfg.mockChapterCount = 2;
    cfg.chaptersPerBatch = 2;
    cfg.mockReviewAlwaysFail = false;

    const QString id = o.startWorkflow(QStringLiteral("novel_progress"), cfg);
    QVERIFY(waitForWorkflow(o, id));

    // 完成后进度应为 100
    QCOMPARE(o.getWorkflowProgress(id), 100);

    // 不存在的 workflowId 应返回 0
    QCOMPARE(o.getWorkflowProgress(QStringLiteral("nonexistent")), 0);
    QVERIFY(!o.isWorkflowFinished(QStringLiteral("nonexistent")));
}

void TestAgentOrchestrator::testAbortWorkflow()
{
    AgentOrchestrator o;
    AgentOrchestrator::WorkflowConfig cfg;
    cfg.mockChapterCount = 6;     // 多章，确保 abort 时机有效
    cfg.chaptersPerBatch = 6;
    cfg.autoReview = false;      // 关掉审校，让流程更快

    bool gotCompletedSignal = false;
    bool capturedSuccess = true;
    QString capturedReason;
    QObject::connect(&o, &AgentOrchestrator::workflowCompleted,
                     this, [&](const QString &, bool ok, const QString &reason) {
        gotCompletedSignal = true;
        capturedSuccess = ok;
        capturedReason = reason;
    });

    // 在第一个 taskStarted 信号触发时调用 abort：
    // mock 工作流是同步执行的，direct connection 让此 lambda 在 executeWorkflow 内
    // emit taskStarted 时立即运行，确保第一个任务已加入列表后再置 aborted=true。
    // 这样下一个步骤的 abort 检查会捕获到，并标记当前任务为 Failed(aborted)。
    bool aborted = false;
    QObject::connect(&o, &AgentOrchestrator::taskStarted,
                     this, [&](const QString &wid, const QVariantMap &) {
        if (!aborted) {
            o.abortWorkflow(wid);
            aborted = true;
        }
    });

    const QString id = o.startWorkflow(QStringLiteral("novel_abort"), cfg);

    QVERIFY(waitForWorkflow(o, id));

    QVERIFY(gotCompletedSignal);
    QVERIFY(!capturedSuccess);
    QCOMPARE(capturedReason, QStringLiteral("aborted"));

    // 任务列表里应该有 Failed 任务（标记 aborted）
    const auto tasks = o.getWorkflowStatus(id);
    QVERIFY(!tasks.isEmpty());
    bool hasAbortedTask = false;
    for (const auto &t : tasks) {
        if (t.status == AgentOrchestrator::TaskStatus::Failed
            && t.error == QStringLiteral("aborted")) {
            hasAbortedTask = true;
            break;
        }
    }
    QVERIFY(hasAbortedTask);
}

void TestAgentOrchestrator::testReviewAlwaysFailRetries()
{
    AgentOrchestrator o;
    AgentOrchestrator::WorkflowConfig cfg;
    cfg.mockChapterCount = 1;
    cfg.chaptersPerBatch = 1;
    cfg.maxRetries = 2;
    cfg.mockReviewAlwaysFail = true;     // 强制审校失败
    cfg.stopOnReviewFail = false;        // 失败后继续（不停止）

    bool capturedSuccess = true;
    QString capturedReason;
    QObject::connect(&o, &AgentOrchestrator::workflowCompleted,
                     this, [&](const QString &, bool ok, const QString &reason) {
        capturedSuccess = ok;
        capturedReason = reason;
    });

    const QString id = o.startWorkflow(QStringLiteral("novel_retry"), cfg);
    QVERIFY(waitForWorkflow(o, id));

    // 工作流结束但不成功
    QVERIFY(!capturedSuccess);

    // 任务列表校验：1 setting + 1 outline + 1 writer + 1 reviewer = 4 个任务
    // （reviewer 失败后虽然重试了 writer，但任务列表里只记录最后一次 reviewer）
    const auto tasks = o.getWorkflowStatus(id);
    QVERIFY(tasks.size() >= 4);

    // 最后一个 reviewer 任务应该 Failed
    bool hasFailedReviewer = false;
    for (const auto &t : tasks) {
        if (t.role == AgentOrchestrator::AgentRole::ReviewerAgent
            && t.status == AgentOrchestrator::TaskStatus::Failed) {
            hasFailedReviewer = true;
            QVERIFY(t.attempt > 0);
            QVERIFY(t.error.contains(QStringLiteral("审校未通过")));
            break;
        }
    }
    QVERIFY(hasFailedReviewer);
}

void TestAgentOrchestrator::testStopOnReviewFail()
{
    AgentOrchestrator o;
    AgentOrchestrator::WorkflowConfig cfg;
    cfg.mockChapterCount = 3;
    cfg.chaptersPerBatch = 3;
    cfg.maxRetries = 1;
    cfg.mockReviewAlwaysFail = true;
    cfg.stopOnReviewFail = true;        // ★ 审校失败立即停止

    QString capturedReason;
    bool capturedSuccess = true;
    QObject::connect(&o, &AgentOrchestrator::workflowCompleted,
                     this, [&](const QString &, bool ok, const QString &reason) {
        capturedSuccess = ok;
        capturedReason = reason;
    });

    const QString id = o.startWorkflow(QStringLiteral("novel_stop"), cfg);
    QVERIFY(waitForWorkflow(o, id));

    QVERIFY(!capturedSuccess);
    QCOMPARE(capturedReason, QStringLiteral("review_failed"));
}

void TestAgentOrchestrator::testSkipAgents()
{
    AgentOrchestrator o;
    AgentOrchestrator::WorkflowConfig cfg;
    cfg.autoSetting = false;     // 跳过设定
    cfg.autoOutline = false;     // 跳过大纲
    cfg.autoWrite = false;       // 跳过写作 + 审校

    const QString id = o.startWorkflow(QStringLiteral("novel_skip"), cfg);
    QVERIFY(waitForWorkflow(o, id));

    const auto tasks = o.getWorkflowStatus(id);
    QVERIFY(!tasks.isEmpty());

    // 所有任务应该是 Skipped
    for (const auto &t : tasks) {
        QCOMPARE(int(t.status), int(AgentOrchestrator::TaskStatus::Skipped));
    }
}

void TestAgentOrchestrator::testConfigFromMap()
{
    QVariantMap m;
    m[QStringLiteral("autoSetting")] = false;
    m[QStringLiteral("autoOutline")] = true;
    m[QStringLiteral("autoWrite")] = false;
    m[QStringLiteral("autoReview")] = true;
    m[QStringLiteral("chaptersPerBatch")] = 5;
    m[QStringLiteral("maxRetries")] = 3;
    m[QStringLiteral("stopOnReviewFail")] = true;
    m[QStringLiteral("mockReviewAlwaysFail")] = true;
    m[QStringLiteral("mockChapterCount")] = 10;

    const auto cfg = AgentOrchestrator::configFromMap(m);
    QCOMPARE(cfg.autoSetting, false);
    QCOMPARE(cfg.autoOutline, true);
    QCOMPARE(cfg.autoWrite, false);
    QCOMPARE(cfg.autoReview, true);
    QCOMPARE(cfg.chaptersPerBatch, 5);
    QCOMPARE(cfg.maxRetries, 3);
    QCOMPARE(cfg.stopOnReviewFail, true);
    QCOMPARE(cfg.mockReviewAlwaysFail, true);
    QCOMPARE(cfg.mockChapterCount, 10);

    // 默认值：空 map
    const auto def = AgentOrchestrator::configFromMap(QVariantMap());
    QCOMPARE(def.autoSetting, true);
    QCOMPARE(def.autoOutline, true);
    QCOMPARE(def.autoWrite, true);
    QCOMPARE(def.autoReview, true);
    QCOMPARE(def.chaptersPerBatch, 3);
    QCOMPARE(def.maxRetries, 2);
    QCOMPARE(def.stopOnReviewFail, false);
    QCOMPARE(def.mockReviewAlwaysFail, false);
    QCOMPARE(def.mockChapterCount, 6);
}

void TestAgentOrchestrator::testTasksToVariantList()
{
    QList<AgentOrchestrator::AgentTask> tasks;
    AgentOrchestrator::AgentTask t1;
    t1.role = AgentOrchestrator::AgentRole::SettingAgent;
    t1.title = QStringLiteral("设定任务");
    t1.chapterId = QString();
    t1.status = AgentOrchestrator::TaskStatus::Succeeded;
    t1.result = QStringLiteral("世界观...");
    t1.error = QString();
    t1.attempt = 0;
    tasks.append(t1);

    AgentOrchestrator::AgentTask t2;
    t2.role = AgentOrchestrator::AgentRole::WriterAgent;
    t2.title = QStringLiteral("写作任务·第1章");
    t2.chapterId = QStringLiteral("ch1");
    t2.status = AgentOrchestrator::TaskStatus::Failed;
    t2.result = QString();
    t2.error = QStringLiteral("审校未通过");
    t2.attempt = 2;
    tasks.append(t2);

    const QVariantList list = AgentOrchestrator::tasksToVariantList(tasks);
    QCOMPARE(list.size(), 2);

    const QVariantMap m1 = list[0].toMap();
    QCOMPARE(m1[QStringLiteral("role")].toInt(), 0);
    QCOMPARE(m1[QStringLiteral("roleLabel")].toString(), QStringLiteral("设定"));
    QCOMPARE(m1[QStringLiteral("title")].toString(), QStringLiteral("设定任务"));
    QCOMPARE(m1[QStringLiteral("status")].toInt(), 2);
    QCOMPARE(m1[QStringLiteral("statusLabel")].toString(), QStringLiteral("成功"));
    QCOMPARE(m1[QStringLiteral("result")].toString(), QStringLiteral("世界观..."));
    QCOMPARE(m1[QStringLiteral("attempt")].toInt(), 0);

    const QVariantMap m2 = list[1].toMap();
    QCOMPARE(m2[QStringLiteral("role")].toInt(), 2);
    QCOMPARE(m2[QStringLiteral("roleLabel")].toString(), QStringLiteral("写作"));
    QCOMPARE(m2[QStringLiteral("chapterId")].toString(), QStringLiteral("ch1"));
    QCOMPARE(m2[QStringLiteral("status")].toInt(), 3);
    QCOMPARE(m2[QStringLiteral("statusLabel")].toString(), QStringLiteral("失败"));
    QCOMPARE(m2[QStringLiteral("error")].toString(), QStringLiteral("审校未通过"));
    QCOMPARE(m2[QStringLiteral("attempt")].toInt(), 2);
}

QTEST_GUILESS_MAIN(TestAgentOrchestrator)
#include "test_agentorchestrator.moc"
