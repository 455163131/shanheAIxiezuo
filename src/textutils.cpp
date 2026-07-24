#include "textutils.h"

#include <QRegularExpression>
#include <QStringList>
#include <cmath>

namespace TextUtils {

// Count "words" = characters excluding whitespace and sentence-ending
// punctuation. For Chinese this matches the usual "zi shu" notion.
int countWords(const QString &text)
{
    static const QRegularExpression nonWord(QStringLiteral("[\\s。！？…]"));
    QString stripped = text;
    stripped.remove(nonWord);
    return stripped.length();
}

QString trimToWordLimit(const QString &text, int maxWords)
{
    if (text.isEmpty() || maxWords <= 0) return QString();
    if (countWords(text) <= maxWords) return text;

    static const QRegularExpression sentenceEnd(QStringLiteral("[。！？…]"));
    QStringList parts = text.split(sentenceEnd, Qt::KeepEmptyParts);

    QString accumulated;
    int wordCount = 0;
    for (int i = 0; i < parts.size(); ++i) {
        const QString &part = parts[i];
        int partWords = countWords(part);
        if (wordCount + partWords > maxWords) break;
        accumulated += part;
        if (i < parts.size() - 1) accumulated += QStringLiteral("。");
        wordCount += partWords;
    }

    if (accumulated.isEmpty() || countWords(accumulated) == 0) {
        QString hard = text;
        while (countWords(hard) > maxWords && !hard.isEmpty()) hard.chop(1);
        return hard;
    }
    return accumulated;
}

int estimateTokens(const QString &text)
{
    return static_cast<int>(std::round(countWords(text) * 0.6));
}

int estimateLoss(int words)
{
    return static_cast<int>(std::round(words * 0.25));
}

QString stripHardcodedWordCount(const QString &text)
{
    if (text.isEmpty()) return text;
    QString out = text;
    static const struct {
        QRegularExpression pattern;
        QString replacement;
    } rules[] = {
        { QRegularExpression(QStringLiteral("正文不少于\\d+字")),         QString() },
        { QRegularExpression(QStringLiteral("正文字数不少于\\d+")),        QString() },
        { QRegularExpression(QStringLiteral("字数不少于\\d+字")),          QString() },
        { QRegularExpression(QStringLiteral("不少于\\d+字")),              QString() },
        { QRegularExpression(QStringLiteral("约\\d+字")),                  QString() },
        { QRegularExpression(QStringLiteral("\\d+字左右")),                QString() },
        { QRegularExpression(QStringLiteral("字数[:：]\\s*\\d+[~-]\\d+")), QString() },
        { QRegularExpression(QStringLiteral("字数[:：]\\s*\\d+")),         QString() },
        { QRegularExpression(QStringLiteral("【([^】]*?)·\\d+】")),       QStringLiteral("【\\1】") },
    };
    for (const auto &rule : rules) out.replace(rule.pattern, rule.replacement);
    return out;
}

} // namespace TextUtils
