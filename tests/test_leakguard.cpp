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
    QString output1 = QStringLiteral("这是正文。# 参考上下文是另一回事。");
    QString prompt = QStringLiteral("# 参考上下文\n故事背景");
    QVERIFY(!LeakGuard::looksLikePromptLeak(output1, prompt));

    QString output2 = QStringLiteral("这是正文。# 参考上下文。【素材声明】不该出现。");
    QVERIFY(LeakGuard::looksLikePromptLeak(output2, prompt));
}

void TestLeakGuard::strongFingerprint_singleHit()
{
    QString output = QStringLiteral("这是正文。【素材声明】你给我素材我来写。");
    QString prompt = QStringLiteral("【素材声明】\n以下为素材");
    QVERIFY(LeakGuard::looksLikePromptLeak(output, prompt));
}

void TestLeakGuard::largeCopyBlocked()
{
    QString prompt = QStringLiteral("从前有座山，山里有座庙，庙里有个老和尚在讲故事。");
    QString output = QStringLiteral("好的，我继续写：") + prompt;
    QVERIFY(LeakGuard::looksLikePromptLeak(output, prompt));
}

void TestLeakGuard::normalOutputNotFlagged()
{
    QString prompt = QStringLiteral("【素材声明】\n素材内容\n# 参考上下文\n背景");
    QString output = QStringLiteral(
        "林凡踏入虚空裂缝，眼前光芒大盛。他感受到一股浩瀚的力量涌入体内，"
        "经脉中的灵力开始疯狂运转。这是突破的征兆——筑基期，即将大成。");
    QVERIFY(!LeakGuard::looksLikePromptLeak(output, prompt));
}

void TestLeakGuard::shortOutputPassthrough()
{
    QString prompt = QStringLiteral("【素材声明】素材");
    QString output = QStringLiteral("短输出");
    QVERIFY(!LeakGuard::looksLikePromptLeak(output, prompt));
}

QTEST_GUILESS_MAIN(TestLeakGuard)
#include "test_leakguard.moc"