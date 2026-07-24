#include "leakguard.h"

#include <QStringList>

namespace LeakGuard {

const QString BLOCK_MESSAGE = QStringLiteral(
    "【内容已拦截】检测到输出中包含提示词防护文本，"
    "本次生成已被安全策略阻止。请尝试调整输入或重新生成。"
);

static const QStringList STRONG_MARKERS = {
    QStringLiteral("【素材声明】"),
    QStringLiteral("硬性规则"),
    QStringLiteral("补充约束"),
    QStringLiteral("你是网文续写助手"),
};

static const QStringList WEAK_MARKERS = {
    QStringLiteral("这是前文的章节内容："),
    QStringLiteral("# 参考上下文"),
    QStringLiteral("[续写要求]："),
    QStringLiteral("【正文字数】"),
    QStringLiteral("再次强调"),
    QStringLiteral("[细纲]："),
};

bool looksLikePromptLeak(const QString &output, const QString &assembledPrompt)
{
    // spec 3.4: outputs shorter than 40 chars are too short to judge reliably.
    if (output.length() < 40) return false;

    // 1. Strong fingerprint: a single hit blocks.
    for (const QString &marker : STRONG_MARKERS) {
        if (output.contains(marker, Qt::CaseSensitive)) return true;
    }

    // 2. Weak fingerprint: need >= 2 distinct hits to block.
    int weakHits = 0;
    for (const QString &marker : WEAK_MARKERS) {
        if (output.contains(marker, Qt::CaseSensitive)) {
            ++weakHits;
            if (weakHits >= 2) return true;
        }
    }

    // 3. Large copy: any 40-char window of the output that verbatim
    // appears in the assembled prompt indicates a leaked fragment.
    if (!assembledPrompt.isEmpty()) {
        const int WINDOW = 40;
        for (int i = 0; i + WINDOW <= output.length(); ++i) {
            QString snippet = output.mid(i, WINDOW);
            if (assembledPrompt.contains(snippet, Qt::CaseSensitive)) return true;
        }
    }

    return false;
}

} // namespace LeakGuard
