#pragma once

#include <QString>

namespace LeakGuard {

// Three-tier prompt leak detector:
//   1) Strong fingerprint markers (single hit -> block)
//   2) Weak fingerprint markers (two+ hits -> block)
//   3) Large copy of assembled prompt substring (40-char window -> block)
// Returns true if the output looks like a prompt leak and should be blocked.
bool looksLikePromptLeak(const QString &output, const QString &assembledPrompt);

// User-facing block message returned when a leak is detected.
extern const QString BLOCK_MESSAGE;

} // namespace LeakGuard