#include <QtTest>
#include "promptassembler.h"

class TestPromptAssembler : public QObject
{
    Q_OBJECT
private slots:
    void sixSegmentOrder();
    void emptyEntityPlaceholder();
    void wordLineAppearsTwice();
    void loadRecentChapters_lastN_tailCut();
    void loadRecentChapters_lastChapters_slidingWindow();
    void loadRecentChapters_manual_selected();
    void loadRecentChapters_none_empty();
    void promptForDisplay_stripsProtection();
};

// Helper: build a fully-populated input so all six segments have content.
static PromptAssembler::AssembleInput makeFullInput()
{
    PromptAssembler::AssembleInput in;
    in.storyBackground = QStringLiteral("九洲大陆，灵气复苏。");
    in.characters.append({
        QStringLiteral("林凡"), QStringLiteral("男"),
        QStringLiteral("坚毅冷静"), QStringLiteral("黑发黑瞳"),
        QStringLiteral("出身寒微的剑修")
    });
    in.characterRelations = QStringLiteral("林凡与苏婉儿亦师亦友");
    in.terms.append({ QStringLiteral("筑基"), QStringLiteral("修行的第一境界") });
    in.memos.append({ QStringLiteral("设定备忘"), QStringLiteral("主角剑名「断水」") });
    in.knowledgeCards.append({ QStringLiteral("剑修流派"),
        QStringLiteral("以内息御剑，注重意念与剑意的契合") });
    in.chapterPlot = QStringLiteral("【起手·240】主角觉醒剑意，初次斩破虚空");
    in.recentChapters.append({
        QStringLiteral("第一章 觉醒"), QStringLiteral("林凡盘膝而坐，灵气在经脉中流转。")
    });
    in.wordCountMin = 2000;
    in.wordCountMax = 2500;
    return in;
}

void TestPromptAssembler::sixSegmentOrder()
{
    using namespace PromptAssembler;
    AssembleInput in = makeFullInput();
    const QString prompt = assemblePrompt(in,
        QStringLiteral("保持原有文风"), QStringLiteral("续写第一章正文"));

    // 6 segments must appear in order. Markers are chosen to be unique to their
    // segment so the banner (which mentions 参考上下文 / 输入内容 / 创作指令
    // in prose) does not produce false matches.
    //   1. banner      -> 【素材声明】
    //   2. recent      -> 这是前文的章节内容：
    //   3. reference   -> # 参考上下文
    //   4. input       -> [续写要求]：  (unique to input section; closing has [续写要求] without colon)
    //   5. closing     -> 补充约束      (unique to closing section)
    //   6. reminder    -> 再次强调：
    int p1 = prompt.indexOf(QStringLiteral("【素材声明】"));
    int p2 = prompt.indexOf(QStringLiteral("这是前文的章节内容："));
    int p3 = prompt.indexOf(QStringLiteral("# 参考上下文"));
    int p4 = prompt.indexOf(QStringLiteral("[续写要求]："));
    int p5 = prompt.indexOf(QStringLiteral("补充约束"));
    int p6 = prompt.indexOf(QStringLiteral("再次强调："));

    QVERIFY(p1 >= 0);
    QVERIFY(p2 > p1);
    QVERIFY(p3 > p2);
    QVERIFY(p4 > p3);
    QVERIFY(p5 > p4);
    QVERIFY(p6 > p5);

    // Word-count line and plot block should both live inside the input section,
    // i.e. after [续写要求]： and before 补充约束 (closing).
    int pWord = prompt.indexOf(QStringLiteral("【正文字数·最高优先级】"));
    int pPlot = prompt.indexOf(QStringLiteral("[细纲]："));
    QVERIFY(pWord > p4 && pWord < p5);
    QVERIFY(pPlot > p4 && pPlot < p5);
}

void TestPromptAssembler::emptyEntityPlaceholder()
{
    using namespace PromptAssembler;
    AssembleInput in;  // all fields empty by default

    const QString prompt = assemblePrompt(in);

    // Empty entities must produce placeholder text so the skeleton stays stable.
    QVERIFY(prompt.contains(QStringLiteral("故事背景：无")));
    QVERIFY(prompt.contains(QStringLiteral("本章角色：无")));
    QVERIFY(prompt.contains(QStringLiteral("人物关系：[无]")));
    QVERIFY(prompt.contains(QStringLiteral("词条信息：[无]")));
    // Empty recent-chapter list -> "（无前文）" inside the recent block.
    QVERIFY(prompt.contains(QStringLiteral("（无前文）")));
    // Empty plot -> default placeholder from assembler.js:221
    QVERIFY(prompt.contains(QStringLiteral("（未填写细纲，请根据前文合理续写）")));
    // No style/requirement template -> default placeholder from assembler.js:165
    QVERIFY(prompt.contains(QStringLiteral("（未选择写作要求）")));
}

void TestPromptAssembler::wordLineAppearsTwice()
{
    using namespace PromptAssembler;
    AssembleInput in = makeFullInput();

    const QString prompt = assemblePrompt(in);
    const QString wordLine = formatWordCountRange(in.wordCountMin, in.wordCountMax);

    QVERIFY(!wordLine.isEmpty());
    // The word line must appear exactly twice: once in [续写要求] (assembler.js:222)
    // and once in the "再次强调：" reminder (assembler.js:226).
    int count = 0;
    int from = 0;
    while (true) {
        int idx = prompt.indexOf(wordLine, from);
        if (idx < 0) break;
        ++count;
        from = idx + wordLine.length();
    }
    QCOMPARE(count, 2);

    // First occurrence sits inside [续写要求]：block; second inside 再次强调：.
    int pReq = prompt.indexOf(QStringLiteral("[续写要求]："));
    int pRem = prompt.indexOf(QStringLiteral("再次强调："));
    QVERIFY(pReq >= 0 && pRem > pReq);
    int pFirst = prompt.indexOf(wordLine);
    int pSecond = prompt.indexOf(wordLine, pFirst + wordLine.length());
    QVERIFY(pFirst > pReq && pFirst < pRem);
    QVERIFY(pSecond > pRem);
}

void TestPromptAssembler::loadRecentChapters_lastN_tailCut()
{
    QVector<PromptAssembler::ChapterRef> allChapters = {
        {QStringLiteral("第1章"), QStringLiteral("第一章正文内容很长很长很长很长很长很长很长很长很长"), QString(), true},
        {QStringLiteral("第2章"), QStringLiteral("第二章正文内容很长很长很长很长很长很长很长很长很长"), QString(), true},
        {QStringLiteral("第3章"), QStringLiteral("第三章正文内容"), QString(), true},
    };

    auto recent = PromptAssembler::loadRecentChapters(allChapters, QStringLiteral("lastN"), 20, 2);
    QVERIFY(recent.size() <= 2);
    if (recent.size() >= 1) {
        QVERIFY(recent.last().title == QStringLiteral("第3章"));
    }
}

void TestPromptAssembler::loadRecentChapters_lastChapters_slidingWindow()
{
    QVector<PromptAssembler::ChapterRef> allChapters = {
        {QStringLiteral("第1章"), QStringLiteral("第一章正文"), QStringLiteral("第一章摘要"), true},
        {QStringLiteral("第2章"), QStringLiteral("第二章正文"), QStringLiteral("第二章摘要"), true},
        {QStringLiteral("第3章"), QStringLiteral("第三章正文"), QStringLiteral("第三章摘要"), true},
    };

    auto recent = PromptAssembler::loadRecentChapters(allChapters, QStringLiteral("lastChapters"), 0, 1);
    QVERIFY(recent.size() >= 1);
    QVERIFY(recent.last().hasContent);
    QVERIFY(!recent.last().content.isEmpty());
    for (int i = 0; i < recent.size() - 1; ++i) {
        QVERIFY(!recent[i].hasContent || recent[i].content.isEmpty());
    }
}

void TestPromptAssembler::loadRecentChapters_manual_selected()
{
    QVector<PromptAssembler::ChapterRef> allChapters = {
        {QStringLiteral("第1章"), QStringLiteral("正文1"), QString(), true},
        {QStringLiteral("第2章"), QStringLiteral("正文2"), QString(), true},
        {QStringLiteral("第3章"), QStringLiteral("正文3"), QString(), true},
    };

    QVector<int> manualIds = {0, 2};
    auto recent = PromptAssembler::loadRecentChapters(allChapters, QStringLiteral("manual"), 0, 0, manualIds);
    QCOMPARE(recent.size(), 2);
    QCOMPARE(recent[0].title, QStringLiteral("第1章"));
    QCOMPARE(recent[1].title, QStringLiteral("第3章"));
}

void TestPromptAssembler::loadRecentChapters_none_empty()
{
    QVector<PromptAssembler::ChapterRef> allChapters = {
        {QStringLiteral("第1章"), QStringLiteral("正文1"), QString(), true},
    };

    auto recent = PromptAssembler::loadRecentChapters(allChapters, QStringLiteral("none"), 0, 0);
    QVERIFY(recent.isEmpty());
}

void TestPromptAssembler::promptForDisplay_stripsProtection()
{
    PromptAssembler::AssembleInput input;
    input.storyBackground = QStringLiteral("测试背景");
    input.wordCountMin = 1000;
    input.wordCountMax = 1500;
    input.emptyPolicy = QStringLiteral("placeholder");

    auto result = PromptAssembler::assemble(input);
    QString display = PromptAssembler::promptForDisplay(result.prompt);

    QVERIFY(!display.contains(QStringLiteral("【素材声明】")));
    QVERIFY(!display.contains(QStringLiteral("硬性规则")));
    QVERIFY(!display.contains(QStringLiteral("【补充约束")));
    QVERIFY(display.contains(QStringLiteral("测试背景")));
    QVERIFY(display.contains(QStringLiteral("1000")));
}

QTEST_GUILESS_MAIN(TestPromptAssembler)
#include "test_promptassembler.moc"
