#include "personas.h"

namespace {

// 函数内静态局部变量：避免静态初始化顺序问题，且元素地址在程序生命周期内稳定，
// 因此 byKey() 返回其指针是安全的。
const QVector<PersonaDef> &table()
{
    static const QVector<PersonaDef> k = {
        PersonaDef{
            QStringLiteral("思考者"),
            QStringLiteral("\n【人格·思考者】重逻辑与人物动机，因果扎实，爽点铺垫充分。"),
            QColor(QStringLiteral("#caa86a"))  // Theme.gold
        },
        PersonaDef{
            QStringLiteral("奇想版"),
            QStringLiteral("\n【人格·奇想版】偏好出人意料的转折与新奇设定，敢于打破套路。"),
            QColor(QStringLiteral("#ff8fb1"))  // Theme.female
        },
        PersonaDef{
            QStringLiteral("氛围版"),
            QStringLiteral("\n【人格·氛围版】重感官与情绪渲染，节奏舒缓，画面感强。"),
            QColor(QStringLiteral("#6ea8fe"))  // Theme.male（统一为蓝，修复此前的颜色分歧）
        },
    };
    return k;
}

} // namespace

namespace Personas {

const QVector<PersonaDef> &all() { return table(); }

QStringList keys()
{
    QStringList out;
    for (const PersonaDef &p : table())
        out.append(p.key);
    return out;
}

const PersonaDef *byKey(const QString &key)
{
    for (const PersonaDef &p : table())
        if (p.key == key)
            return &p;
    return table().isEmpty() ? nullptr : &table().first();
}

QString systemPrompt(const QString &key)
{
    const PersonaDef *p = byKey(key);
    return p ? p->systemPrompt : QString();
}

QColor color(const QString &key)
{
    const PersonaDef *p = byKey(key);
    return p ? p->color : QColor(QStringLiteral("#9aa4b2"));  // Theme.sub
}

} // namespace Personas
