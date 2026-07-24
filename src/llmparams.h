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

} // namespace LlmParams