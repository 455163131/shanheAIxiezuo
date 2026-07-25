#include <QtTest>
#include "consistencychecker.h"

#include <QList>
#include <QVector>

class TestConsistencyChecker : public QObject
{
    Q_OBJECT
private slots:
    void characterGenderMismatch();
    void characterGenderCorrect();
    void characterNameTypo();
    void timelineContradiction();
    void timelineNormal();
    void termViolation();
    void termCorrect();
    void unresolvedForeshadowing();
    void resolvedForeshadowing();
    void personMixFirstThird();
    void styleConsistent();
    void checkAllAggregation();
    void emptyInput();
    void noCharacters();

private:
    // 辅助：判断 issue 列表中是否存在指定 type + severity 的项
    static bool hasIssue(const QList<ConsistencyIssue> &issues,
                         const QString &type,
                         const QString &severity);
    static bool hasIssueType(const QList<ConsistencyIssue> &issues,
                             const QString &type);
};

bool TestConsistencyChecker::hasIssue(const QList<ConsistencyIssue> &issues,
                                      const QString &type,
                                      const QString &severity)
{
    for (const ConsistencyIssue &i : issues) {
        if (i.type == type && i.severity == severity)
            return true;
    }
    return false;
}

bool TestConsistencyChecker::hasIssueType(const QList<ConsistencyIssue> &issues,
                                          const QString &type)
{
    for (const ConsistencyIssue &i : issues) {
        if (i.type == type)
            return true;
    }
    return false;
}

// 1. 角色设定女性，正文用"他" → 检出 warning
void TestConsistencyChecker::characterGenderMismatch()
{
    ConsistencyChecker checker;
    ConsistencyInput input;
    input.chapterId = "ch01";
    input.chapterText = QStringLiteral("林婉儿走进了大殿，她抬眼望去，只见前方站着一个身影。"
                                       "林婉儿他心中一惊，转身便走。");

    EntityRef c;
    c.id = "c1";
    c.name = "林婉儿";
    c.type = "character";
    c.gender = "女";
    input.characters.append(c);

    QList<ConsistencyIssue> issues = checker.checkCharacterConsistency(input);
    QVERIFY(hasIssue(issues, "character", "warning"));
}

// 2. 角色设定女性，正文用"她" → 无问题
void TestConsistencyChecker::characterGenderCorrect()
{
    ConsistencyChecker checker;
    ConsistencyInput input;
    input.chapterId = "ch01";
    input.chapterText = QStringLiteral("林婉儿走进了大殿，她抬眼望去，只见前方站着一个身影。");

    EntityRef c;
    c.id = "c1";
    c.name = "林婉儿";
    c.type = "character";
    c.gender = "女";
    input.characters.append(c);

    QList<ConsistencyIssue> issues = checker.checkCharacterConsistency(input);
    // 不应报性别冲突类 warning（注意：可能仍报错别字类 warning，需精确匹配 detail）
    for (const ConsistencyIssue &i : issues) {
        if (i.title.contains(QStringLiteral("性别代词"))) {
            QFAIL("不应报告性别代词冲突");
        }
    }
}

// 3. 角色名"沈青云"，正文写成"沈庆云" → 检出 warning
void TestConsistencyChecker::characterNameTypo()
{
    ConsistencyChecker checker;
    ConsistencyInput input;
    input.chapterId = "ch02";
    input.chapterText = QStringLiteral("沈庆云拔剑而出，剑光如霜。沈青云也拔剑而出。");

    EntityRef c;
    c.id = "c1";
    c.name = "沈青云";
    c.type = "character";
    c.gender = "男";
    input.characters.append(c);

    QList<ConsistencyIssue> issues = checker.checkCharacterConsistency(input);
    bool foundTypo = false;
    for (const ConsistencyIssue &i : issues) {
        if (i.title.contains(QStringLiteral("错别字")))
            foundTypo = true;
    }
    QVERIFY(foundTypo);
}

// 4. "昨天出发"+"次日出发"描述同一事件 → 检出 error
void TestConsistencyChecker::timelineContradiction()
{
    ConsistencyChecker checker;
    ConsistencyInput input;
    input.chapterId = "ch03";
    input.chapterText = QStringLiteral("昨天出发前往青云宗。"
                                       "经过一番波折，次日出发的队伍终于也抵达了。");

    QList<ConsistencyIssue> issues = checker.checkTimelineConsistency(input);
    QVERIFY(hasIssue(issues, "timeline", "error"));
}

// 5. 正常时间线 → 无问题
void TestConsistencyChecker::timelineNormal()
{
    ConsistencyChecker checker;
    ConsistencyInput input;
    input.chapterId = "ch04";
    input.chapterText = QStringLiteral("昨天我去了青云宗，今天回来，明天再去。");

    QList<ConsistencyIssue> issues = checker.checkTimelineConsistency(input);
    QVERIFY(issues.isEmpty());
}

// 6. 词条定义"青云宗在东方"，正文写"青云宗在西边" → 检出 warning
void TestConsistencyChecker::termViolation()
{
    ConsistencyChecker checker;
    ConsistencyInput input;
    input.chapterId = "ch05";
    input.chapterText = QStringLiteral("我们一路向青云宗走去，青云宗在西边山巅之上。");

    EntityRef t;
    t.id = "t1";
    t.name = "青云宗";
    t.type = "term";
    t.summary = QStringLiteral("青云宗位于东方的苍穹山巅。");
    input.terms.append(t);

    QList<ConsistencyIssue> issues = checker.checkTermConsistency(input);
    QVERIFY(hasIssue(issues, "term", "warning"));
}

// 7. 词条定义与正文一致 → 无问题
void TestConsistencyChecker::termCorrect()
{
    ConsistencyChecker checker;
    ConsistencyInput input;
    input.chapterId = "ch06";
    input.chapterText = QStringLiteral("我们一路向青云宗走去，青云宗在东方山巅之上。");

    EntityRef t;
    t.id = "t1";
    t.name = "青云宗";
    t.type = "term";
    t.summary = QStringLiteral("青云宗位于东方的苍穹山巅。");
    input.terms.append(t);

    QList<ConsistencyIssue> issues = checker.checkTermConsistency(input);
    QVERIFY(issues.isEmpty());
}

// 8. 大纲标记伏笔，正文未提及 → 检出 info
void TestConsistencyChecker::unresolvedForeshadowing()
{
    ConsistencyChecker checker;
    ConsistencyInput input;
    input.chapterId = "ch07";
    input.chapterText = QStringLiteral("主角登场，未见任何异常。");

    EntityRef o;
    o.id = "o1";
    o.name = "第1章 开篇";
    o.type = "outline";
    o.content = QStringLiteral("伏笔：神秘黑剑");
    input.outlines.append(o);

    QList<ConsistencyIssue> issues = checker.checkPlotConsistency(input);
    QVERIFY(hasIssue(issues, "plot", "info"));
}

// 9. 大纲标记伏笔，正文提及 → 无问题
void TestConsistencyChecker::resolvedForeshadowing()
{
    ConsistencyChecker checker;
    ConsistencyInput input;
    input.chapterId = "ch08";
    input.chapterText = QStringLiteral("主角在山洞中发现了一把神秘黑剑，剑身寒气逼人。");

    EntityRef o;
    o.id = "o1";
    o.name = "第1章 开篇";
    o.type = "outline";
    o.content = QStringLiteral("伏笔：神秘黑剑");
    input.outlines.append(o);

    QList<ConsistencyIssue> issues = checker.checkPlotConsistency(input);
    QVERIFY(issues.isEmpty());
}

// 10. 第一人称和第三人称混用 → 检出 warning
void TestConsistencyChecker::personMixFirstThird()
{
    ConsistencyChecker checker;
    ConsistencyInput input;
    input.chapterId = "ch09";
    // 第一人称和第三人称都 >= 3 次
    input.chapterText = QStringLiteral(
        "我走进房间，我看到他在那里，我想他应该是累了。"
        "我喊了他一声，他没有回应。我心里有些不安。");

    QList<ConsistencyIssue> issues = checker.checkStyleConsistency(input);
    QVERIFY(hasIssue(issues, "style", "warning"));
}

// 11. 风格一致 → 无问题
void TestConsistencyChecker::styleConsistent()
{
    ConsistencyChecker checker;
    ConsistencyInput input;
    input.chapterId = "ch10";
    // 纯第三人称、纯现代风格
    input.chapterText = QStringLiteral(
        "他走进房间，他看到桌子上有一封信。"
        "他拆开信封，他读了起来。");

    QList<ConsistencyIssue> issues = checker.checkStyleConsistency(input);
    QVERIFY(issues.isEmpty());
}

// 12. 综合检查聚合所有问题
void TestConsistencyChecker::checkAllAggregation()
{
    ConsistencyChecker checker;
    ConsistencyInput input;
    input.chapterId = "ch11";
    // 同时包含：性别冲突 + 时间矛盾 + 词条矛盾 + 未回收伏笔 + 人称混用
    input.chapterText = QStringLiteral(
        "林婉儿他昨日前往青云宗，"
        "次日她又动身了。"
        "青云宗在西边山巅。"
        "我看见她在那里，她也看见了我，我心里有些不安，她心里也有些不安。");

    EntityRef c;
    c.id = "c1";
    c.name = "林婉儿";
    c.type = "character";
    c.gender = "女";
    input.characters.append(c);

    EntityRef t;
    t.id = "t1";
    t.name = "青云宗";
    t.type = "term";
    t.summary = QStringLiteral("青云宗位于东方。");
    input.terms.append(t);

    EntityRef o;
    o.id = "o1";
    o.name = "第1章";
    o.type = "outline";
    o.content = QStringLiteral("伏笔：神秘黑剑");
    input.outlines.append(o);

    QList<ConsistencyIssue> issues = checker.checkAll(input);
    // 至少聚合了多个类别
    QVERIFY(issues.size() >= 4);
    QVERIFY(hasIssueType(issues, "character"));
    QVERIFY(hasIssueType(issues, "timeline"));
    QVERIFY(hasIssueType(issues, "term"));
}

// 13. 空输入 → 无问题
void TestConsistencyChecker::emptyInput()
{
    ConsistencyChecker checker;
    ConsistencyInput input;
    input.chapterId = "ch12";
    input.chapterText = "";

    QVERIFY(checker.checkAll(input).isEmpty());
    QVERIFY(checker.checkCharacterConsistency(input).isEmpty());
    QVERIFY(checker.checkTimelineConsistency(input).isEmpty());
    QVERIFY(checker.checkTermConsistency(input).isEmpty());
    QVERIFY(checker.checkPlotConsistency(input).isEmpty());
    QVERIFY(checker.checkStyleConsistency(input).isEmpty());
}

// 14. 无角色设定 → 跳过角色检查（不崩溃，返回空）
void TestConsistencyChecker::noCharacters()
{
    ConsistencyChecker checker;
    ConsistencyInput input;
    input.chapterId = "ch13";
    input.chapterText = QStringLiteral("他走进了大殿，她迎了上来。");
    // 不添加任何角色设定

    QList<ConsistencyIssue> issues = checker.checkCharacterConsistency(input);
    QVERIFY(issues.isEmpty());
}

QTEST_GUILESS_MAIN(TestConsistencyChecker)
#include "test_consistencychecker.moc"
