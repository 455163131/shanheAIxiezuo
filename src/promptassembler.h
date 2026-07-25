#pragma once

#include <QString>
#include <QStringList>
#include <QVector>

namespace PromptAssembler {

// Material entity structs (mirror project1 assembler.js field shapes).
// Aggregates so they support brace initialization in tests.
struct ChapterRef {
    QString title;
    QString content;
};

struct CharacterRef {
    QString name;
    QString gender;
    QString personality;
    QString appearance;
    QString description;
};

struct TermRef {
    QString name;
    QString content;
};

struct MemoRef {
    QString title;
    QString content;
};

struct KnowledgeCardRef {
    QString title;
    QString content;
};

// All material fields needed to assemble a prompt.
// Mirrors project1 assembler.js assemblePrompt input shape.
struct AssembleInput {
    QString storyBackground;
    QVector<CharacterRef> characters;
    QString characterRelations;
    QVector<TermRef> terms;
    QVector<MemoRef> memos;
    QVector<KnowledgeCardRef> knowledgeCards;
    QString chapterPlot;
    QVector<ChapterRef> recentChapters;
    int wordCountMin = 2000;
    int wordCountMax = 2500;
};

// Sanitized word-count range: clamp to [100, 20000], hi < lo -> hi = lo.
// Ported from assembler.js:190-199.
struct WordRange { int lo; int hi; };
WordRange sanitizeWordRange(int min, int max);

// Format the word-count line. Appears twice in the assembled prompt:
// once inside [续写要求] and once in the "再次强调：" reminder.
// Ported from assembler.js:201-204.
QString formatWordCountRange(int min, int max);

// Strip hardcoded word-count directives from templates / plots / knowledge
// cards. Delegates to TextUtils::stripHardcodedWordCount.
QString stripHardcodedWordCount(const QString &text);

// Format the recent-chapters body (without the "这是前文的章节内容：" prefix).
// Empty list -> "（无前文）". Ported from assembler.js:154-161.
QString formatRecentChapters(const QVector<ChapterRef> &chapters);

// Build the reference-context blocks (storyBackground, characters, relations,
// terms, memos, knowledgeCards). Empty entities produce placeholder text so
// the skeleton stays stable; memos/knowledgeCards omit the block when empty
// to avoid clutter (matches assembler.js:143-144). Ported from
// assembler.js:124-147.
QStringList buildReferenceBlocks(const AssembleInput &input);

// Format the [续写要求] template portion (style + requirement template joined,
// hardcoded word counts stripped). Both empty -> "（未选择写作要求）".
// Ported from assembler.js:163-168.
QString formatRequirement(const QString &styleTemplate, const QString &requirementTemplate);

// Load a prompt fragment from qrc; on failure return the fallback literal.
// Tests do not link the qrc, so the fallback must contain the protection text
// (【素材声明】 / 补充约束) that LeakGuard looks for.
QString loadPromptFile(const QString &qrcPath, const QString &fallback);

// Assemble the full 6-segment prompt (assembler.js:211-231):
//   1. MATERIAL_BANNER      (content/prompts/banner.txt)
//   2. recentBlock          "这是前文的章节内容：\n<recent>"
//   3. referenceSection     "# 参考上下文\n<blocks joined by \n>"
//   4. inputSection         "输入内容\n\n[续写要求]：\n<wordLine>\n\n<reqText>\n\n[细纲]：\n<plotText>"
//   5. CLOSING_INSTRUCTION  (content/prompts/closing.txt)
//   6. wordReminder         "再次强调：<wordLine>"
// Segments are joined with "\n\n".
QString assemblePrompt(const AssembleInput &input,
                       const QString &styleTemplate = QString(),
                       const QString &requirementTemplate = QString());

// Assemble a preview (display) variant: identical to assemblePrompt except
// segment 5 uses closing_for_display.txt — the internal "补充约束" /
// anti-exfiltration wording is redacted so the preview does not expose
// protection rules to the UI.
QString assemblePromptForDisplay(const AssembleInput &input,
                                 const QString &styleTemplate = QString(),
                                 const QString &requirementTemplate = QString());

} // namespace PromptAssembler
