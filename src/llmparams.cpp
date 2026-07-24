#include "llmparams.h"

#include <QRegularExpression>
#include <algorithm>
#include <cmath>

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

SamplingFields buildSampling(int creativityIndex, bool thinkingAuto,
                             int thinkingIndex, int maxTokens)
{
    SamplingFields s;
    int ci = std::clamp(creativityIndex, 0, 5);
    s.temperature = CREATIVITY[ci];

    int ti = std::clamp(thinkingIndex, 0, 4);
    int presetBudget = THINKING[ti].budget;
    int maxTokens25 = static_cast<int>(std::floor(maxTokens * 0.25));
    int budget = std::min({presetBudget, maxTokens25, THINKING_HARD_CAP});
    s.thinkingBudget = std::max(256, budget);

    if (!thinkingAuto) {
        s.reasoningEffort = QString::fromLatin1(THINKING[ti].effort);
    }

    return s;
}

int tokensForWordBudget(int targetWords, int thinkingBudget,
                        int userMaxTokens, double slack)
{
    int base = static_cast<int>(std::ceil(targetWords * slack));
    int total = base + 320 + thinkingBudget;
    total = std::max(512, std::min(total, MAX_TOKENS_HARD_CAP));

    if (userMaxTokens > 0) {
        total = std::min(total, userMaxTokens);
        total = std::max(512, total);
    }

    return total;
}

SamplingFields softenSampling(const SamplingFields &src, const QString &model,
                             bool thinkingAuto, int thinkingIndex)
{
    SamplingFields out = src;

    // Rule 1: reasoner model -> drop all params (temperature/reasoning_effort/thinking_budget)
    if (isReasonerModel(model)) {
        out.reasoningEffort = std::nullopt;
        out.thinkingBudget = std::nullopt;
        out.temperature = -1.0;  // sentinel: HttpLlmClient skips temperature when < 0
        return out;
    }

    // Rule 2: non-reasoner + manual high/xhigh/max (thinkingAuto=false, thinkingIndex>=2)
    //         -> drop reasoning_effort/thinking_budget, keep temperature
    if (!thinkingAuto && thinkingIndex >= 2) {
        out.reasoningEffort = std::nullopt;
        out.thinkingBudget = std::nullopt;
        return out;
    }

    // Rule 3: others -> no softening
    return out;
}

} // namespace LlmParams