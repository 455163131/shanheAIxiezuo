#include "llmparams.h"

#include <QRegularExpression>

namespace LlmParams {

bool isReasonerModel(const QString &modelName)
{
    if (modelName.isEmpty()) return false;
    // Ported from project1 ai.js:190.
    // Matches: deepseek-r1, deepseek-reasoner, qwq, o1, o3, o4-mini,
    // step-3-thinking, model-r1-distill, etc.
    static const QRegularExpression re(
        QStringLiteral("reasoner|deepseek-r1|\\br1\\b|thinking|qwq|o1\\b|o3\\b|o4-mini"),
        QRegularExpression::CaseInsensitiveOption
    );
    return re.match(modelName).hasMatch();
}

} // namespace LlmParams