#include <QtTest>
#include "textutils.h"

class TestTextUtils : public QObject
{
    Q_OBJECT
private slots:
    void countWords();
    void trimToWordLimit();
    void trimToWordLimit_hardCut();
    void estimateTokens();
    void estimateLoss();
    void stripHardcodedWordCount();
    void stripHardcodedWordCount_boundary();
};

void TestTextUtils::countWords()
{
    QCOMPARE(TextUtils::countWords(QStringLiteral("")), 0);
    QCOMPARE(TextUtils::countWords(QStringLiteral("  \n\t ")), 0);
    QCOMPARE(TextUtils::countWords(QStringLiteral("hello world")), 10);
    QCOMPARE(TextUtils::countWords(QStringLiteral("你好世界")), 4);
    QCOMPARE(TextUtils::countWords(QStringLiteral("  你好  世界  ")), 4);
    QCOMPARE(TextUtils::countWords(QStringLiteral("第一段。\n第二段。")), 8);
}

void TestTextUtils::trimToWordLimit()
{
    QString text1 = QStringLiteral("第一句。第二句。第三句。第四句。");
    QString result1 = TextUtils::trimToWordLimit(text1, 6);
    QVERIFY(result1 == QStringLiteral("第一句。")
            || result1 == QStringLiteral("第一句。第二句。"));
    QString text2 = QStringLiteral("短文本");
    QCOMPARE(TextUtils::trimToWordLimit(text2, 100), QStringLiteral("短文本"));
    QCOMPARE(TextUtils::trimToWordLimit(QStringLiteral(""), 100), QStringLiteral(""));
}

void TestTextUtils::trimToWordLimit_hardCut()
{
    QString longText = QStringLiteral("这是一段没有句号的长文本它会一直持续下去超过字数限制");
    QString result = TextUtils::trimToWordLimit(longText, 10);
    QVERIFY(result.length() <= longText.length());
    QVERIFY(TextUtils::countWords(result) <= 10);
}

void TestTextUtils::estimateTokens()
{
    QCOMPARE(TextUtils::estimateTokens(QStringLiteral("hello")), 3);
    QCOMPARE(TextUtils::estimateTokens(QStringLiteral("你好世界")), 2);
    QCOMPARE(TextUtils::estimateTokens(QStringLiteral("")), 0);
}

void TestTextUtils::estimateLoss()
{
    QCOMPARE(TextUtils::estimateLoss(100), 25);
    QCOMPARE(TextUtils::estimateLoss(8), 2);
    QCOMPARE(TextUtils::estimateLoss(0), 0);
}

void TestTextUtils::stripHardcodedWordCount()
{
    QString tpl1 = QStringLiteral("请续写正文，不少于2000字，保持风格。");
    QCOMPARE(TextUtils::stripHardcodedWordCount(tpl1),
             QStringLiteral("请续写正文，，保持风格。"));

    QString tpl2 = QStringLiteral("正文字数不少于2500，注意节奏。");
    QCOMPARE(TextUtils::stripHardcodedWordCount(tpl2),
             QStringLiteral("，注意节奏。"));

    QString tpl3 = QStringLiteral("【起手·240】主角觉醒系统");
    QCOMPARE(TextUtils::stripHardcodedWordCount(tpl3),
             QStringLiteral("【起手】主角觉醒系统"));

    QString tpl4 = QStringLiteral("不少于2000字。字数：2000~2500。【高潮·500】");
    QString result4 = TextUtils::stripHardcodedWordCount(tpl4);
    QVERIFY(!result4.contains(QStringLiteral("2000")));
    QVERIFY(!result4.contains(QStringLiteral("2500")));
    QVERIFY(!result4.contains(QStringLiteral("500")));
    QVERIFY(result4.contains(QStringLiteral("【高潮】")));
}

void TestTextUtils::stripHardcodedWordCount_boundary()
{
    QString clean = QStringLiteral("请保持原有写作风格。");
    QCOMPARE(TextUtils::stripHardcodedWordCount(clean), clean);
    QCOMPARE(TextUtils::stripHardcodedWordCount(QStringLiteral("")),
             QStringLiteral(""));

    QString withNumber = QStringLiteral("第3章：主角18岁。");
    QString result = TextUtils::stripHardcodedWordCount(withNumber);
    QVERIFY(result.contains(QStringLiteral("第3章")));
    QVERIFY(result.contains(QStringLiteral("18岁")));
}

QTEST_GUILESS_MAIN(TestTextUtils)
#include "test_textutils.moc"
