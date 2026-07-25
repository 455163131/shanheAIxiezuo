#pragma once

#include <QList>
#include <QVector>
#include <QString>
#include <QJsonObject>
#include <QJsonArray>
#include <QVariantMap>

/**
 * Book / Chapter domain models (pure data structures + JSON serialization).
 *
 * Design intent: In the initial project, books and chapters only lived in QML
 * in-memory arrays, lost on restart, and were untestable. Here we沉淀 the
 * domain model as serializable C++ structures as the unified foundation for
 * later persistence (QSettings / SQLite) and QML exposure (wrapped in
 * QAbstractListModel). At this stage we only provide toJson/fromJson and do
 * not force QML integration, to avoid breaking the existing build.
 *
 * Note: We deliberately avoid Q_GADGET here to skip moc dependency; when the
 * team is ready to expose this to QML, wrap it in a QObject list model and
 * register the metatype.
 */
struct Chapter
{
    QString m_title;
    QString m_content;
    QString m_summary;
    QString m_summarySource;
    int m_sortOrder = 0;
    QString m_status;
    int m_wordCount = 0;
    QVariantMap m_aiConfig;
    QVector<int> m_linkedCharacters;
    QVector<int> m_linkedTerms;
    QVector<int> m_linkedKnowledge;
    QVector<int> m_linkedMemos;
    QVector<int> m_linkedOutlines;

    QJsonObject toJson() const
    {
        QJsonObject o;
        o["title"] = m_title;
        o["content"] = m_content;
        o["summary"] = m_summary;
        o["summarySource"] = m_summarySource;
        o["sortOrder"] = m_sortOrder;
        o["status"] = m_status;
        o["wordCount"] = m_wordCount;
        o["aiConfig"] = QJsonObject::fromVariantMap(m_aiConfig);

        QJsonArray linkedChars;
        for (int id : m_linkedCharacters)
            linkedChars.append(id);
        o["linkedCharacters"] = linkedChars;

        QJsonArray linkedTerms;
        for (int id : m_linkedTerms)
            linkedTerms.append(id);
        o["linkedTerms"] = linkedTerms;

        QJsonArray linkedKnowledge;
        for (int id : m_linkedKnowledge)
            linkedKnowledge.append(id);
        o["linkedKnowledge"] = linkedKnowledge;

        QJsonArray linkedMemos;
        for (int id : m_linkedMemos)
            linkedMemos.append(id);
        o["linkedMemos"] = linkedMemos;

        QJsonArray linkedOutlines;
        for (int id : m_linkedOutlines)
            linkedOutlines.append(id);
        o["linkedOutlines"] = linkedOutlines;

        return o;
    }

    static Chapter fromJson(const QJsonObject &o)
    {
        Chapter c;
        c.m_title = o.value("title").toString();
        c.m_content = o.value("content").toString();
        c.m_summary = o.value("summary").toString();
        c.m_summarySource = o.value("summarySource").toString();
        c.m_sortOrder = o.value("sortOrder").toInt();
        c.m_status = o.value("status").toString();
        c.m_wordCount = o.value("wordCount").toInt();
        c.m_aiConfig = o.value("aiConfig").toObject().toVariantMap();

        const QJsonArray linkedChars = o.value("linkedCharacters").toArray();
        for (const QJsonValue &v : linkedChars)
            c.m_linkedCharacters.append(v.toInt());

        const QJsonArray linkedTerms = o.value("linkedTerms").toArray();
        for (const QJsonValue &v : linkedTerms)
            c.m_linkedTerms.append(v.toInt());

        const QJsonArray linkedKnowledge = o.value("linkedKnowledge").toArray();
        for (const QJsonValue &v : linkedKnowledge)
            c.m_linkedKnowledge.append(v.toInt());

        const QJsonArray linkedMemos = o.value("linkedMemos").toArray();
        for (const QJsonValue &v : linkedMemos)
            c.m_linkedMemos.append(v.toInt());

        const QJsonArray linkedOutlines = o.value("linkedOutlines").toArray();
        for (const QJsonValue &v : linkedOutlines)
            c.m_linkedOutlines.append(v.toInt());

        return c;
    }
};

struct Book
{
    QString m_id;
    QString m_title;
    QString m_genreId;
    QList<Chapter> m_chapters;
    QVariantMap m_prefs;

    QJsonObject toJson() const
    {
        QJsonObject o;
        o["id"] = m_id;
        o["title"] = m_title;
        o["genreId"] = m_genreId;
        o["prefs"] = QJsonObject::fromVariantMap(m_prefs);
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
        b.m_prefs = o.value("prefs").toObject().toVariantMap();
        const QJsonArray ch = o.value("chapters").toArray();
        for (const QJsonValue &v : ch)
            b.m_chapters.append(Chapter::fromJson(v.toObject()));
        return b;
    }

    QJsonObject toMetaJson() const
    {
        QJsonObject o;
        o["id"] = m_id;
        o["title"] = m_title;
        o["genreId"] = m_genreId;
        o["prefs"] = QJsonObject::fromVariantMap(m_prefs);
        return o;
    }

    static Book fromMetaJson(const QJsonObject &o)
    {
        Book b;
        b.m_id = o.value("id").toString();
        b.m_title = o.value("title").toString();
        b.m_genreId = o.value("genreId").toString();
        b.m_prefs = o.value("prefs").toObject().toVariantMap();
        return b;
    }
};