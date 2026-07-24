#include <QtTest>
#include "llmparams.h"

class TestLlmParams : public QObject
{
    Q_OBJECT
private slots:
    void isReasonerModel_matches();
    void isReasonerModel_notMatches();
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

QTEST_GUILESS_MAIN(TestLlmParams)
#include "test_llmparams.moc"
