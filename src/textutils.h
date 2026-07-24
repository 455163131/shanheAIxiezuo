#pragma once

#include <QString>

namespace TextUtils {

int countWords(const QString &text);
QString trimToWordLimit(const QString &text, int maxWords);
int estimateTokens(const QString &text);
int estimateLoss(int words);
QString stripHardcodedWordCount(const QString &text);

} // namespace TextUtils
