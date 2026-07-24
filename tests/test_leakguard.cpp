#include <QtTest>
#include "leakguard.h"

class TestLeakGuard : public QObject
{
    Q_OBJECT
private slots:
    void weakFingerprint_accumulatedTwo();
    void strongFingerprint_singleHit();
    void largeCopyBlocked();
    void normalOutputNotFlagged();
    void shortOutputPassthrough();
};

void TestLeakGuard::weakFingerprint_accumulatedTwo()
{
    // Weak markers: 1 hit does not block; need >= 2 hits.
    // output1 has 1 weak marker, length > 40, should NOT block.
    QString output1 = QStringLiteral("这是正文的开始部分，主角正在山中修炼，忽然天空传来一声巨响。# 参考上下文是另一回事。");
    QString prompt = QStringLiteral("# 参考上下文\n故事背景");
    QVERIFY(!LeakGuard::looksLikePromptLeak(output1, prompt));

    // output2 has 2 weak markers, length > 40, should block.
    QString output2 = QStringLiteral("这是正文的开始部分，主角正在山中修炼，忽然天空传来一声巨响。# 参考上下文。【素材声明】不该出现。");
    QVERIFY(LeakGuard::looksLikePromptLeak(output2, prompt));
}

void TestLeakGuard::strongFingerprint_singleHit()
{
    // Strong marker: 1 hit blocks (output length must be > 40).
    QString output = QStringLiteral("这是正文的开始部分，主角正在山中修炼，忽然天空传来一声巨响。【素材声明】你给我素材我来写。");
    QString prompt = QStringLiteral("【素材声明】\n以下为素材");
    QVERIFY(LeakGuard::looksLikePromptLeak(output, prompt));
}

void TestLeakGuard::largeCopyBlocked()
{
    // Large verbatim copy: prompt > 40 chars, output contains prompt verbatim > 40 chars.
    QString prompt = QStringLiteral("从前有座山，山里有座庙，庙里有个老和尚在讲故事，讲的是什么故事呢，原来是山海经里的故事。");
    QString output = QStringLiteral("好的，我继续写：") + prompt;
    QVERIFY(LeakGuard::looksLikePromptLeak(output, prompt));
}

void TestLeakGuard::normalOutputNotFlagged()
{
    // Normal prose output is not flagged.
    QString prompt = QStringLiteral("【素材声明】\n素材内容\n# 参考上下文\n背景");
    QString output = QStringLiteral(
        "林凡踏入虚空裂缝，眼前光芒大盛。他感受到一股浩瀚的力量涌入体内，"
        "经脉中的灵力开始疯狂运转。这是突破的征兆——筑基期，即将大成。");
    QVERIFY(!LeakGuard::looksLikePromptLeak(output, prompt));
}

void TestLeakGuard::shortOutputPassthrough()
{
    // Output < 40 chars passes through even if it contains a strong marker.
    QString prompt = QStringLiteral("【素材声明】素材");
    QString output = QStringLiteral("短输出含【素材声明】但太短");  // < 40 chars
    QVERIFY(!LeakGuard::looksLikePromptLeak(output, prompt));
}

QTEST_GUILESS_MAIN(TestLeakGuard)
#include "test_leakguard.moc"