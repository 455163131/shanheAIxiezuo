#pragma once

#include <QColor>
#include <QString>
#include <QStringList>
#include <QVector>

/**
 * 人格（路由）单一真相源。
 *
 * 之前「思考者 / 奇想版 / 氛围版」的魔法字符串在 C++（buildSystemPrompt 的
 * if/else 比较）与 QML（ComboBox model、对比视图列表、WorkflowPanel 颜色映射）
 * 多处重复硬编码，且衍生出颜色不一致的隐藏 bug（氛围版在 WorkflowPanel 是蓝、
 * 在 CompareView 是粉）。
 *
 * 现统一收敛到此处：新增 / 修改人格只改这一个文件。
 *   - key          : 稳定标识，与 genres.json 的 persona 字段、QML 显示保持一致
 *   - systemPrompt : 注入 system prompt 的人格指令（可为空）
 *   - color        : 关联主题色（统一来源，根治颜色不一致）
 *
 * 注意：key 使用中文显示名而非英文枚举值，是为了与 genres.json 数据字段直接对齐，
 * 避免再引入一份「key↔显示名」映射。代码内部比较一律走 key。
 */
struct PersonaDef {
    QString key;
    QString systemPrompt;
    QColor  color;
};

enum class PersonaId { Thinker, Whimsy, Atmosphere };

namespace Personas {

/// 全部人格定义（顺序即默认展示顺序）
const QVector<PersonaDef> &all();

/// 人格 key 列表，供 ComboBox / Repeater / 数组绑定
QStringList keys();

/// 按 key 查找；找不到回落到首个（思考者）
const PersonaDef *byKey(const QString &key);

/// 人格对应的 system prompt 片段；找不到返回空串
QString systemPrompt(const QString &key);

/// 人格关联主题色；找不到回落到中性灰（Theme.sub）
QColor color(const QString &key);

} // namespace Personas
