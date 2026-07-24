#pragma once

#include <QString>
#include <optional>

namespace LlmParams {

// Sampling-related fields shared across model providers.
// Kept together so callers can pass a single struct around.
struct SamplingFields {
    double temperature = 0.9;
    std::optional<QString> reasoningEffort;
    std::optional<int> thinkingBudget;
};

// Creativity presets mapped to the UI creativity slider (0..5).
// Index 0 = most conservative, index 5 = most adventurous.
static const double CREATIVITY[6] = {0.7, 0.8, 0.9, 1.0, 1.1, 1.2};

// Thinking presets for reasoner models: effort label + token budget.
// Index 0..4 correspond to the UI thinking-level selector.
struct ThinkingPreset {
    const char *effort;
    int budget;
};
static const ThinkingPreset THINKING[5] = {
    {"low",    1600},
    {"medium", 3200},
    {"high",   4800},
    {"xhigh", 6400},
    {"max",    8192},
};

// Hard caps shared across providers to avoid runaway token budgets.
static constexpr int THINKING_HARD_CAP = 8192;
static constexpr int MAX_TOKENS_HARD_CAP = 65536;

// Returns true if the given model name matches the reasoner pattern
// (deepseek-r1 / reasoner / r1 / thinking / qwq / o1 / o3 / o4-mini).
// Ported from project1 ai.js:190.
bool isReasonerModel(const QString &modelName);

// Builds sampling fields from UI indices.
// creativityIndex 0..5 -> CREATIVITY[i]; thinkingIndex 0..4 -> THINKING[i].
// thinkingBudget is capped at 25% of maxTokens and THINKING_HARD_CAP, with a
// floor of 256. When thinkingAuto is true, reasoningEffort is left unset
// (the provider decides automatically).
SamplingFields buildSampling(int creativityIndex, bool thinkingAuto,
                             int thinkingIndex, int maxTokens);

// Reverse-derives a max-token budget from a target word count.
// Formula: ceil(words * slack) + 320 + thinkingBudget, clamped to
// [512, MAX_TOKENS_HARD_CAP]. When userMaxTokens > 0, the result is
// further capped by it (but never below 512).
int tokensForWordBudget(int targetWords, int thinkingBudget,
                        int userMaxTokens, double slack = 1.35);

// Softens sampling fields based on model type and thinking-level selection.
// Rule 1: reasoner model -> drop reasoning_effort/thinking_budget and set
//         temperature to -1 (sentinel: HttpLlmClient skips temperature < 0).
// Rule 2: non-reasoner + manual high/xhigh/max (thinkingAuto=false,
//         thinkingIndex>=2) -> drop reasoning_effort/thinking_budget, keep
//         temperature untouched.
// Rule 3: everything else (auto mode, manual low/medium) -> no softening.
SamplingFields softenSampling(const SamplingFields &src, const QString &model,
                             bool thinkingAuto, int thinkingIndex);

} // namespace LlmParams