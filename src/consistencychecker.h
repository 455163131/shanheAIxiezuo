#pragma once

#include <QString>
#include <QList>
#include <QVector>

// EntityRef: 设定/前文实体的轻量引用，供 ConsistencyChecker 使用。
// 与 EntityStore 的 QVariantMap 解耦——上层负责从 QVariantMap 抽出必要字段填入此处。
struct EntityRef {
    QString id;          // 实体 ID
    QString name;        // 名称（角色名/词条名/卡名/大纲标题）
    QString type;        // character / term / knowledge / outline
    QString gender;      // 仅角色有效：男/女/其他（空表示未设定）
    QString summary;     // 简短摘要/定义（用于词条矛盾检查）
    QString content;     // 完整内容（用于伏笔/承诺文本检索）
};

// ConsistencyIssue: 一致性问题清单的一条记录。
struct ConsistencyIssue {
    QString type;        // character / timeline / term / plot / style
    QString severity;    // error / warning / info
    QString title;       // 问题标题
    QString detail;      // 详细描述
    QString suggestion;  // 修改建议
    QString location;    // 定位（章节ID + 段落索引或行号）
    QString evidence;    // 证据文本（矛盾的原文片段）
};

// ConsistencyInput: 一致性检查的输入聚合。
struct ConsistencyInput {
    QString chapterText;           // 当前章节正文
    QString chapterId;
    QString previousText;          // 前文（可选，用于跨章检查）
    QVector<EntityRef> characters; // 角色设定列表
    QVector<EntityRef> terms;      // 词条设定列表
    QVector<EntityRef> knowledge;  // 知识卡列表
    QVector<EntityRef> outlines;   // 大纲列表
};

// ConsistencyChecker: 纯规则的一致性审校引擎。
// 不调用 LLM，可离线运行；扫描正文与设定/前文的矛盾，返回结构化问题清单。
// 借鉴 AI_NovelGenerator 的"一致性审校"环节，落地为可被 QML 调用的 C++ 内核。
class ConsistencyChecker
{
public:
    ConsistencyChecker() = default;

    // 1. 角色一致性：检查正文中角色性别/外貌/性格是否与设定矛盾
    QList<ConsistencyIssue> checkCharacterConsistency(const ConsistencyInput &input);

    // 2. 时间线一致性：检查时间描述前后矛盾（"昨天"/"三天前"/"次年"等）
    QList<ConsistencyIssue> checkTimelineConsistency(const ConsistencyInput &input);

    // 3. 词条一致性：检查词条定义与正文用法是否矛盾
    QList<ConsistencyIssue> checkTermConsistency(const ConsistencyInput &input);

    // 4. 伏笔追踪：检查大纲中的伏笔/承诺是否在正文中回收
    QList<ConsistencyIssue> checkPlotConsistency(const ConsistencyInput &input);

    // 5. 风格一致性：检查人称/时态/文风是否前后一致
    QList<ConsistencyIssue> checkStyleConsistency(const ConsistencyInput &input);

    // 综合检查（调用以上所有）
    QList<ConsistencyIssue> checkAll(const ConsistencyInput &input);

private:
    // 在 text 中查找 name 出现的所有位置索引
    static QList<int> findAllOccurrences(const QString &text, const QString &name);

    // 提取 name 出现位置附近的窗口文本（前后 N 字符），用于代词/属性匹配
    static QString windowAround(const QString &text, int pos, int windowSize);

    // 简易字符相似度（编辑距离 / max(len)），用于错别字近似匹配
    static double similarity(const QString &a, const QString &b);
};
