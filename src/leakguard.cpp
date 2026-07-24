#include "leakguard.h"
#include <QStringList>

namespace LeakGuard {

const QString BLOCK_MESSAGE = QStringLiteral(
    "【内容已拦截】检测到输出中包含提示词防护文本，"
    "本次生成已被安全策略阻止。请尝试调整输入或重新生成。"
);

// Strong markers: high-precision fingerprints of the guard text itself.
// A single occurrence is enough to flag the output as a leak.
static const QStringList STRONG_MARKERS = {
    QStringLiteral("【素材声明】"),
    QStringLiteral("硬性规则"),
    QStringLiteral("补充约束"),
    QStringLiteral("你是网文续写助手"),
};

// Weak markers: lower-precision fragments that may appear in normal prose.
// Require at least two distinct hits before flagging.
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
    // Tier 1: strong fingerprint - single hit blocks.
    // Strong markers are high-precision; even very short outputs that contain
    // them should be flagged, so no length guard here.
    for (const QString &marker : STRONG_MARKERS) {
        if (output.contains(marker, Qt::CaseSensitive)) return true;
    }

    // Tier 2: weak fingerprint - need two or more hits to block.
    // Two-hit rule alone is enough to suppress false positives on short text.
    int weakHits = 0;
    for (const QString &marker : WEAK_MARKERS) {
        if (output.contains(marker, Qt::CaseSensitive)) {
            ++weakHits;
            if (weakHits >= 2) return true;
        }
    }

    // Tier 3: large copy - any 20-char window of the output that verbatim
    // appears in the assembled prompt indicates a leaked fragment.
    // Window smaller than the shortest realistic prompt fragment so that
    // real leaks of even short prompts are caught.
    if (!assembledPrompt.isEmpty()) {
        const int WINDOW = 20;
        for (int i = 0; i + WINDOW <= output.length(); ++i) {
            QString snippet = output.mid(i, WINDOW);
            if (assembledPrompt.contains(snippet, Qt::CaseSensitive)) return true;
        }
    }

    return false;
}

} // namespace LeakGuard