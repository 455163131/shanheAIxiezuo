#include <QtTest>
#include "llmparams.h"

class TestLlmParams : public QObject
{
    Q_OBJECT
private slots:
    void isReasonerModel_matches();
    void isReasonerModel_notMatches();
    void buildSampling_creativityMapping();
    void buildSampling_thinkingBudget25PercentCap();
    void buildSampling_autoMode();
    void tokensForWordBudget_default();
    void tokensForWordBudget_clamp();
    void tokensForWordBudget_continueSlack();
    void softenSampling_reasonerModel();
    void softenSampling_manualHigh();
    void softenSampling_manualLow();
    void softenSampling_autoNotSoftened();
};

void TestLlmParams::isReasonerModel_matches()
{
    QVERIFY(LlmParams::isReasonerModel(QStringLiteral("deepseek-r1")));
    QVERIFY(LlmParams::isReasonerModel(QStringLiteral("deepseek-reasoner")));
    QVERIFY(LlmParams::isReasonerModel(QStringLiteral("qwq-32b")));
    QVERIFY(LlmParams::isReasonerModel(QStringLiteral("o1")));
    QVERIFY(LlmParams::isReasonerModel(QStringLiteral("o1-preview")));
    QVERIFY(LlmParams::isReasonerModel(QStringLiteral("o3")));
    QVERIFY(LlmParams::isReasonerModel(QStringLiteral("o3-mini")));
    QVERIFY(LlmParams::isReasonerModel(QStringLiteral("o4-mini")));
    QVERIFY(LlmParams::isReasonerModel(QStringLiteral("Qwen/QwQ-32B")));
    QVERIFY(LlmParams::isReasonerModel(QStringLiteral("step-3-thinking")));
    QVERIFY(LlmParams::isReasonerModel(QStringLiteral("model-r1-distill")));
}

void TestLlmParams::isReasonerModel_notMatches()
{
    QVERIFY(!LlmParams::isReasonerModel(QStringLiteral("gpt-4o")));
    QVERIFY(!LlmParams::isReasonerModel(QStringLiteral("gpt-4o-mini")));
    QVERIFY(!LlmParams::isReasonerModel(QStringLiteral("deepseek-chat")));
    QVERIFY(!LlmParams::isReasonerModel(QStringLiteral("claude-3-5-sonnet")));
    QVERIFY(!LlmParams::isReasonerModel(QStringLiteral("qwen-max")));
    QVERIFY(!LlmParams::isReasonerModel(QStringLiteral("step-2-16k")));
    QVERIFY(!LlmParams::isReasonerModel(QStringLiteral("gpt-4-turbo")));
}

void TestLlmParams::buildSampling_creativityMapping()
{
    for (int i = 0; i < 6; ++i) {
        auto s = LlmParams::buildSampling(i, true, 2, 4000);
        QCOMPARE(s.temperature, LlmParams::CREATIVITY[i]);
    }
    QCOMPARE(LlmParams::buildSampling(0, true, 2, 4000).temperature, 0.7);
    QCOMPARE(LlmParams::buildSampling(5, true, 2, 4000).temperature, 1.2);
}

void TestLlmParams::buildSampling_thinkingBudget25PercentCap()
{
    // maxTokens=4000 -> thinking cap = 1000
    auto s = LlmParams::buildSampling(3, false, 4, 4000);
    QVERIFY(s.thinkingBudget.has_value());
    QVERIFY(s.thinkingBudget.value() <= 1000);
    QVERIFY(s.thinkingBudget.value() >= 256);

    // maxTokens=40000 -> thinking cap = 10000, but HARD_CAP 8192
    auto s2 = LlmParams::buildSampling(3, false, 4, 40000);
    QVERIFY(s2.thinkingBudget.value() <= 8192);
    QVERIFY(s2.thinkingBudget.value() >= 256);

    // maxTokens=800 -> thinking cap = 200, floor 256
    auto s3 = LlmParams::buildSampling(3, false, 2, 800);
    QCOMPARE(s3.thinkingBudget.value(), 256);
}

void TestLlmParams::buildSampling_autoMode()
{
    auto s = LlmParams::buildSampling(3, true, 2, 4000);
    QVERIFY(s.thinkingBudget.has_value());
    QVERIFY(!s.reasoningEffort.has_value());
}

void TestLlmParams::tokensForWordBudget_default()
{
    // ceil(words * slack) + 320 + thinkingBudget, clamp [512, 65536]
    // words=2000, thinkingBudget=3200, slack=1.35 -> 2700 + 320 + 3200 = 6220
    int result = LlmParams::tokensForWordBudget(2000, 3200, 0, 1.35);
    QCOMPARE(result, 6220);
}

void TestLlmParams::tokensForWordBudget_clamp()
{
    // words=0, thinkingBudget=0 -> 320, clamp to 512
    QCOMPARE(LlmParams::tokensForWordBudget(0, 0, 0, 1.35), 512);

    // words=50000, thinkingBudget=8192 -> 76012, clamp to 65536
    QCOMPARE(LlmParams::tokensForWordBudget(50000, 8192, 0, 1.35), 65536);

    // userMaxTokens limits
    QCOMPARE(LlmParams::tokensForWordBudget(2000, 3200, 4000, 1.35), 4000);
}

void TestLlmParams::tokensForWordBudget_continueSlack()
{
    // continue: slack=1.5, thinkingBudget=0
    // words=1000 -> 1500 + 320 + 0 = 1820
    QCOMPARE(LlmParams::tokensForWordBudget(1000, 0, 0, 1.5), 1820);
}

void TestLlmParams::softenSampling_reasonerModel()
{
    auto src = LlmParams::buildSampling(3, false, 2, 4000);
    QVERIFY(src.thinkingBudget.has_value());
    QVERIFY(src.reasoningEffort.has_value());

    auto softened = LlmParams::softenSampling(src, QStringLiteral("deepseek-r1"), false, 2);
    QVERIFY(!softened.thinkingBudget.has_value());
    QVERIFY(!softened.reasoningEffort.has_value());
    QVERIFY(softened.temperature < 0);  // sentinel -1 means do not send temperature
}

void TestLlmParams::softenSampling_manualHigh()
{
    auto src = LlmParams::buildSampling(3, false, 3, 4000);
    QVERIFY(src.thinkingBudget.has_value());
    QVERIFY(src.reasoningEffort.has_value());

    auto softened = LlmParams::softenSampling(src, QStringLiteral("deepseek-chat"), false, 3);
    QVERIFY(!softened.thinkingBudget.has_value());
    QVERIFY(!softened.reasoningEffort.has_value());
    QCOMPARE(softened.temperature, src.temperature);
}

void TestLlmParams::softenSampling_manualLow()
{
    auto src = LlmParams::buildSampling(3, false, 1, 4000);
    auto softened = LlmParams::softenSampling(src, QStringLiteral("deepseek-chat"), false, 1);
    QCOMPARE(softened.thinkingBudget.has_value(), src.thinkingBudget.has_value());
    QCOMPARE(softened.reasoningEffort.has_value(), src.reasoningEffort.has_value());
    QCOMPARE(softened.temperature, src.temperature);
}

void TestLlmParams::softenSampling_autoNotSoftened()
{
    auto src = LlmParams::buildSampling(3, true, 4, 4000);
    auto softened = LlmParams::softenSampling(src, QStringLiteral("deepseek-chat"), true, 4);
    QCOMPARE(softened.thinkingBudget.has_value(), src.thinkingBudget.has_value());
    QVERIFY(!softened.reasoningEffort.has_value());
}

QTEST_GUILESS_MAIN(TestLlmParams)
#include "test_llmparams.moc"
