#pragma once

#include <QList>
#include <QString>
#include <QJsonObject>
#include <QJsonArray>

/**
 * 书籍 / 章节领域模型（纯数据结构 + JSON 序列化）。
 *
 * 设计意图：初版项目中书籍、章节只存在于 QML 的内存数组里，重启即丢、
 * 且无法单测。这里先把领域模型沉淀为可序列化的 C++ 结构，作为后续
 * 持久化（QSettings / SQLite）与向 QML 暴露（QAbstractListModel 包装）
 * 的统一基石。当前阶段仅提供 toJson/fromJson，不强制接 QML，避免破坏
 * 现有构建。
 *
 * 注：此处刻意不使用 Q_GADGET，以免引入 moc 依赖；当团队准备把它暴露
 * 给 QML 时，再包一层 QObject 列表模型并注册元类型即可。
 */
struct Chapter
{
    QString m_title;
    QString m_content;

    QJsonObject toJson() const
    {
        QJsonObject o;
        o["title"] = m_title;
        o["content"] = m_content;
        return o;
    }

    static Chapter fromJson(const QJsonObject &o)
    {
        Chapter c;
        c.m_title = o.value("title").toString();
        c.m_content = o.value("content").toString();
        return c;
    }
};

struct Book
{
    QString m_id;
    QString m_title;
    QString m_genreId;
    QList<Chapter> m_chapters;

    QJsonObject toJson() const
    {
        QJsonObject o;
        o["id"] = m_id;
        o["title"] = m_title;
        o["genreId"] = m_genreId;
        QJsonArray ch;
        for (const Chapter &c : m_chapters)
            ch.append(c.toJson());
        o["chapters"] = ch;
        return o;
    }

    static Book fromJson(const QJsonObject &o)
    {
        Book b;
        b.m_id = o.value("id").toString();
        b.m_title = o.value("title").toString();
        b.m_genreId = o.value("genreId").toString();
        const QJsonArray ch = o.value("chapters").toArray();
        for (const QJsonValue &v : ch)
            b.m_chapters.append(Chapter::fromJson(v.toObject()));
        return b;
    }
};
