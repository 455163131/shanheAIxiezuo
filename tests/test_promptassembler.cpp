#include <QtTest>
#include "promptassembler.h"

class TestPromptAssembler : public QObject
{
    Q_OBJECT
private slots:
    void sixSegmentOrder();
    void emptyEntityPlaceholder();
    void wordLineAppearsTwice();
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

QTEST_GUILESS_MAIN(TestPromptAssembler)
#include "test_promptassembler.moc"
