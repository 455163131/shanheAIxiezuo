#include "entitystore.h"
#include "textutils.h"

#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFileInfo>
#include <QTextStream>

namespace {
constexpr int SCHEMA_VERSION = 1;

bool writeJsonFile(const QString &path, const QVariant &v)
{
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate))
        return false;
    const QJsonDocument doc = QJsonDocument::fromVariant(v);
    return f.write(doc.toJson(QJsonDocument::Indented)) >= 0;
}

QVariant readJsonFile(const QString &path)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly))
        return QVariant();
    const QJsonDocument doc = QJsonDocument::fromJson(f.readAll());
    return doc.toVariant();
}
} // namespace

EntityStore::EntityStore(const QString &bookDir)
    : m_bookDir(bookDir)
{
    ensureDir();
}

void EntityStore::ensureDir()
{
    QDir dir(m_bookDir);
    if (!dir.exists())
        dir.mkpath(QStringLiteral("."));

    const QStringList files = {
        QStringLiteral("characters.json"),
        QStringLiteral("terms.json"),
        QStringLiteral("knowledge.json"),
        QStringLiteral("memos.json"),
        QStringLiteral("outlines.json")
    };
    for (const QString &fn : files) {
        const QString fullPath = m_bookDir + QLatin1Char('/') + fn;
        if (!QFile::exists(fullPath)) {
            QVariantMap root;
            root[QStringLiteral("schemaVersion")] = SCHEMA_VERSION;
            root[QStringLiteral("items")] = QVariantList();
            if (fn == QStringLiteral("characters.json"))
                root[QStringLiteral("folders")] = QVariantList();
            writeJsonFile(fullPath, root);
        }
    }
}

QString EntityStore::globalTemplatesPath()
{
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
           + QStringLiteral("/global_templates.json");
}

QString EntityStore::globalKnowledgePath()
{
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
           + QStringLiteral("/global_knowledge.json");
}

int EntityStore::nextId(const QVariantList &items)
{
    int maxId = 0;
    for (const QVariant &v : items) {
        const QVariantMap m = v.toMap();
        const int id = m.value(QStringLiteral("id")).toInt();
        if (id > maxId)
            maxId = id;
    }
    return maxId + 1;
}

int EntityStore::autoWordCount(const QString &content)
{
    return TextUtils::countWords(content);
}

QVariantList EntityStore::loadJsonList(const QString &filename, const QString &key) const
{
    const QString path = m_bookDir + QLatin1Char('/') + filename;
    const QVariant v = readJsonFile(path);
    if (v.type() == QVariant::List) {
        return v.toList();
    }
    const QVariantMap m = v.toMap();
    return m.value(key).toList();
}

void EntityStore::saveJsonList(const QString &filename, const QString &key, const QVariantList &items) const
{
    const QString path = m_bookDir + QLatin1Char('/') + filename;
    QVariantMap root;
    root[QStringLiteral("schemaVersion")] = SCHEMA_VERSION;
    root[key] = items;
    writeJsonFile(path, root);
}

QVariantList EntityStore::loadJsonFolders(const QString &filename) const
{
    const QString path = m_bookDir + QLatin1Char('/') + filename;
    const QVariant v = readJsonFile(path);
    const QVariantMap m = v.toMap();
    return m.value(QStringLiteral("folders")).toList();
}

void EntityStore::saveJsonWithFolders(const QString &filename, const QVariantList &items, const QVariantList &folders) const
{
    const QString path = m_bookDir + QLatin1Char('/') + filename;
    QVariantMap root;
    root[QStringLiteral("schemaVersion")] = SCHEMA_VERSION;
    root[QStringLiteral("items")] = items;
    root[QStringLiteral("folders")] = folders;
    writeJsonFile(path, root);
}

QVariantList EntityStore::loadGlobalJsonList(const QString &path, const QString &key)
{
    const QVariant v = readJsonFile(path);
    if (v.type() == QVariant::List) {
        return v.toList();
    }
    const QVariantMap m = v.toMap();
    return m.value(key).toList();
}

void EntityStore::saveGlobalJsonList(const QString &path, const QString &key, const QVariantList &items)
{
    QDir().mkpath(QFileInfo(path).path());
    QVariantMap root;
    root[QStringLiteral("schemaVersion")] = SCHEMA_VERSION;
    root[key] = items;
    writeJsonFile(path, root);
}

// ---- Characters ----

QVariantList EntityStore::characters() const
{
    return loadJsonList(QStringLiteral("characters.json"), QStringLiteral("items"));
}

void EntityStore::setCharacters(const QVariantList &chars)
{
    QVariantList folders = characterFolders();
    saveJsonWithFolders(QStringLiteral("characters.json"), chars, folders);
}

int EntityStore::addCharacter(const QVariantMap &charData)
{
    QVariantList items = characters();
    QVariantMap m = charData;
    const int id = nextId(items);
    m[QStringLiteral("id")] = id;
    if (m.contains(QStringLiteral("content")) && !m.contains(QStringLiteral("wordCount")))
        m[QStringLiteral("wordCount")] = autoWordCount(m.value(QStringLiteral("content")).toString());
    items.append(m);
    setCharacters(items);
    return id;
}

void EntityStore::updateCharacter(int id, const QVariantMap &charData)
{
    QVariantList items = characters();
    for (int i = 0; i < items.size(); ++i) {
        QVariantMap m = items[i].toMap();
        if (m.value(QStringLiteral("id")).toInt() == id) {
            QVariantMap updated = charData;
            updated[QStringLiteral("id")] = id;
            if (updated.contains(QStringLiteral("content")))
                updated[QStringLiteral("wordCount")] = autoWordCount(updated.value(QStringLiteral("content")).toString());
            else if (m.contains(QStringLiteral("wordCount")))
                updated[QStringLiteral("wordCount")] = m.value(QStringLiteral("wordCount"));
            items[i] = updated;
            break;
        }
    }
    setCharacters(items);
}

void EntityStore::deleteCharacter(int id)
{
    QVariantList items = characters();
    QVariantList filtered;
    for (const QVariant &v : items) {
        if (v.toMap().value(QStringLiteral("id")).toInt() != id)
            filtered.append(v);
    }
    setCharacters(filtered);
}

QVariantList EntityStore::characterFolders() const
{
    return loadJsonFolders(QStringLiteral("characters.json"));
}

void EntityStore::setCharacterFolders(const QVariantList &folders)
{
    QVariantList items = characters();
    saveJsonWithFolders(QStringLiteral("characters.json"), items, folders);
}

// ---- Terms ----

QVariantList EntityStore::terms() const
{
    return loadJsonList(QStringLiteral("terms.json"), QStringLiteral("items"));
}

void EntityStore::setTerms(const QVariantList &terms)
{
    saveJsonList(QStringLiteral("terms.json"), QStringLiteral("items"), terms);
}

int EntityStore::addTerm(const QVariantMap &termData)
{
    QVariantList items = terms();
    QVariantMap m = termData;
    const int id = nextId(items);
    m[QStringLiteral("id")] = id;
    if (m.contains(QStringLiteral("content")) && !m.contains(QStringLiteral("wordCount")))
        m[QStringLiteral("wordCount")] = autoWordCount(m.value(QStringLiteral("content")).toString());
    items.append(m);
    setTerms(items);
    return id;
}

void EntityStore::updateTerm(int id, const QVariantMap &termData)
{
    QVariantList items = terms();
    for (int i = 0; i < items.size(); ++i) {
        QVariantMap m = items[i].toMap();
        if (m.value(QStringLiteral("id")).toInt() == id) {
            QVariantMap updated = termData;
            updated[QStringLiteral("id")] = id;
            if (updated.contains(QStringLiteral("content")))
                updated[QStringLiteral("wordCount")] = autoWordCount(updated.value(QStringLiteral("content")).toString());
            else if (m.contains(QStringLiteral("wordCount")))
                updated[QStringLiteral("wordCount")] = m.value(QStringLiteral("wordCount"));
            items[i] = updated;
            break;
        }
    }
    setTerms(items);
}

void EntityStore::deleteTerm(int id)
{
    QVariantList items = terms();
    QVariantList filtered;
    for (const QVariant &v : items) {
        if (v.toMap().value(QStringLiteral("id")).toInt() != id)
            filtered.append(v);
    }
    setTerms(filtered);
}

// ---- Knowledge ----

QVariantList EntityStore::knowledgeCards() const
{
    return loadJsonList(QStringLiteral("knowledge.json"), QStringLiteral("items"));
}

void EntityStore::setKnowledgeCards(const QVariantList &cards)
{
    saveJsonList(QStringLiteral("knowledge.json"), QStringLiteral("items"), cards);
}

int EntityStore::addKnowledgeCard(const QVariantMap &cardData)
{
    QVariantList items = knowledgeCards();
    QVariantMap m = cardData;
    const int id = nextId(items);
    m[QStringLiteral("id")] = id;
    if (m.contains(QStringLiteral("content")) && !m.contains(QStringLiteral("wordCount")))
        m[QStringLiteral("wordCount")] = autoWordCount(m.value(QStringLiteral("content")).toString());
    items.append(m);
    setKnowledgeCards(items);

    if (m.value(QStringLiteral("isGlobal")).toBool())
        addGlobalKnowledge(m);

    return id;
}

void EntityStore::updateKnowledgeCard(int id, const QVariantMap &cardData)
{
    QVariantList items = knowledgeCards();
    for (int i = 0; i < items.size(); ++i) {
        QVariantMap m = items[i].toMap();
        if (m.value(QStringLiteral("id")).toInt() == id) {
            QVariantMap updated = cardData;
            updated[QStringLiteral("id")] = id;
            if (updated.contains(QStringLiteral("content")))
                updated[QStringLiteral("wordCount")] = autoWordCount(updated.value(QStringLiteral("content")).toString());
            else if (m.contains(QStringLiteral("wordCount")))
                updated[QStringLiteral("wordCount")] = m.value(QStringLiteral("wordCount"));
            if (!updated.contains(QStringLiteral("isGlobal")))
                updated[QStringLiteral("isGlobal")] = m.value(QStringLiteral("isGlobal"));
            items[i] = updated;
            break;
        }
    }
    setKnowledgeCards(items);
}

void EntityStore::deleteKnowledgeCard(int id)
{
    QVariantList items = knowledgeCards();
    QVariantList filtered;
    for (const QVariant &v : items) {
        if (v.toMap().value(QStringLiteral("id")).toInt() != id)
            filtered.append(v);
    }
    setKnowledgeCards(filtered);
}

// ---- Memos ----

QVariantList EntityStore::memos() const
{
    return loadJsonList(QStringLiteral("memos.json"), QStringLiteral("items"));
}

void EntityStore::setMemos(const QVariantList &memos)
{
    saveJsonList(QStringLiteral("memos.json"), QStringLiteral("items"), memos);
}

int EntityStore::addMemo(const QVariantMap &memoData)
{
    QVariantList items = memos();
    QVariantMap m = memoData;
    const int id = nextId(items);
    m[QStringLiteral("id")] = id;
    if (m.contains(QStringLiteral("content")) && !m.contains(QStringLiteral("wordCount")))
        m[QStringLiteral("wordCount")] = autoWordCount(m.value(QStringLiteral("content")).toString());
    items.append(m);
    setMemos(items);
    return id;
}

void EntityStore::updateMemo(int id, const QVariantMap &memoData)
{
    QVariantList items = memos();
    for (int i = 0; i < items.size(); ++i) {
        QVariantMap m = items[i].toMap();
        if (m.value(QStringLiteral("id")).toInt() == id) {
            QVariantMap updated = memoData;
            updated[QStringLiteral("id")] = id;
            if (updated.contains(QStringLiteral("content")))
                updated[QStringLiteral("wordCount")] = autoWordCount(updated.value(QStringLiteral("content")).toString());
            else if (m.contains(QStringLiteral("wordCount")))
                updated[QStringLiteral("wordCount")] = m.value(QStringLiteral("wordCount"));
            items[i] = updated;
            break;
        }
    }
    setMemos(items);
}

void EntityStore::deleteMemo(int id)
{
    QVariantList items = memos();
    QVariantList filtered;
    for (const QVariant &v : items) {
        if (v.toMap().value(QStringLiteral("id")).toInt() != id)
            filtered.append(v);
    }
    setMemos(filtered);
}

// ---- Outlines ----

QVariantList EntityStore::outlines() const
{
    return loadJsonList(QStringLiteral("outlines.json"), QStringLiteral("items"));
}

void EntityStore::setOutlines(const QVariantList &outlines)
{
    saveJsonList(QStringLiteral("outlines.json"), QStringLiteral("items"), outlines);
}

int EntityStore::addOutline(const QVariantMap &outlineData)
{
    QVariantList items = outlines();
    QVariantMap m = outlineData;
    const int id = nextId(items);
    m[QStringLiteral("id")] = id;
    if (m.contains(QStringLiteral("content")) && !m.contains(QStringLiteral("wordCount")))
        m[QStringLiteral("wordCount")] = autoWordCount(m.value(QStringLiteral("content")).toString());
    items.append(m);
    setOutlines(items);
    return id;
}

void EntityStore::updateOutline(int id, const QVariantMap &outlineData)
{
    QVariantList items = outlines();
    for (int i = 0; i < items.size(); ++i) {
        QVariantMap m = items[i].toMap();
        if (m.value(QStringLiteral("id")).toInt() == id) {
            QVariantMap updated = outlineData;
            updated[QStringLiteral("id")] = id;
            if (updated.contains(QStringLiteral("content")))
                updated[QStringLiteral("wordCount")] = autoWordCount(updated.value(QStringLiteral("content")).toString());
            else if (m.contains(QStringLiteral("wordCount")))
                updated[QStringLiteral("wordCount")] = m.value(QStringLiteral("wordCount"));
            items[i] = updated;
            break;
        }
    }
    setOutlines(items);
}

void EntityStore::deleteOutline(int id)
{
    QVariantList items = outlines();
    QVariantList filtered;
    for (const QVariant &v : items) {
        if (v.toMap().value(QStringLiteral("id")).toInt() != id)
            filtered.append(v);
    }
    setOutlines(filtered);
}

// ---- Global Templates ----

QVariantList EntityStore::globalTemplates()
{
    return loadGlobalJsonList(globalTemplatesPath(), QStringLiteral("items"));
}

void EntityStore::setGlobalTemplates(const QVariantList &templates)
{
    saveGlobalJsonList(globalTemplatesPath(), QStringLiteral("items"), templates);
}

int EntityStore::addGlobalTemplate(const QVariantMap &tmplData)
{
    QVariantList items = globalTemplates();
    QVariantMap m = tmplData;
    const int id = nextId(items);
    m[QStringLiteral("id")] = id;
    items.append(m);
    setGlobalTemplates(items);
    return id;
}

void EntityStore::updateGlobalTemplate(int id, const QVariantMap &tmplData)
{
    QVariantList items = globalTemplates();
    for (int i = 0; i < items.size(); ++i) {
        QVariantMap m = items[i].toMap();
        if (m.value(QStringLiteral("id")).toInt() == id) {
            QVariantMap updated = tmplData;
            updated[QStringLiteral("id")] = id;
            items[i] = updated;
            break;
        }
    }
    setGlobalTemplates(items);
}

void EntityStore::deleteGlobalTemplate(int id)
{
    QVariantList items = globalTemplates();
    QVariantList filtered;
    for (const QVariant &v : items) {
        if (v.toMap().value(QStringLiteral("id")).toInt() != id)
            filtered.append(v);
    }
    setGlobalTemplates(filtered);
}

void EntityStore::seedTemplatesIfEmpty()
{
    if (!globalTemplates().isEmpty())
        return;

    QVariantList templates;

    QVariantMap style1;
    style1[QStringLiteral("name")] = QStringLiteral("保持原风");
    style1[QStringLiteral("content")] = QStringLiteral("请保持原文的写作风格和语气进行续写。");
    style1[QStringLiteral("category")] = QStringLiteral("style");
    templates.append(style1);

    QVariantMap style2;
    style2[QStringLiteral("name")] = QStringLiteral("白话文");
    style2[QStringLiteral("content")] = QStringLiteral("请用通俗易懂的白话文风格进行续写，语言平实自然。");
    style2[QStringLiteral("category")] = QStringLiteral("style");
    templates.append(style2);

    QVariantMap style3;
    style3[QStringLiteral("name")] = QStringLiteral("番茄爽文风");
    style3[QStringLiteral("content")] = QStringLiteral("请用番茄小说爽文风格续写，节奏快、冲突强、打脸爽。");
    style3[QStringLiteral("category")] = QStringLiteral("style");
    templates.append(style3);

    QVariantMap req1;
    req1[QStringLiteral("name")] = QStringLiteral("续写6.0");
    req1[QStringLiteral("content")] = QStringLiteral("请续写正文，不少于2000字，保持剧情连贯。");
    req1[QStringLiteral("category")] = QStringLiteral("requirement");
    templates.append(req1);

    QVariantMap req2;
    req2[QStringLiteral("name")] = QStringLiteral("橙子一键续写");
    req2[QStringLiteral("content")] = QStringLiteral("请根据前文内容自然续写，注重人物刻画和情节推进。");
    req2[QStringLiteral("category")] = QStringLiteral("requirement");
    templates.append(req2);

    setGlobalTemplates(templates);
}

QVariantMap EntityStore::templateById(int id)
{
    const QVariantList items = globalTemplates();
    for (const QVariant &v : items) {
        const QVariantMap m = v.toMap();
        if (m.value(QStringLiteral("id")).toInt() == id)
            return m;
    }
    return QVariantMap();
}

// ---- Global Knowledge ----

QVariantList EntityStore::globalKnowledgeCards()
{
    return loadGlobalJsonList(globalKnowledgePath(), QStringLiteral("items"));
}

void EntityStore::addGlobalKnowledge(const QVariantMap &cardData)
{
    QVariantList items = globalKnowledgeCards();
    QVariantMap m = cardData;
    items.append(m);
    saveGlobalJsonList(globalKnowledgePath(), QStringLiteral("items"), items);
}

// ---- Chapter Meta ----

QString EntityStore::chapterFileName(int chapterNumber)
{
    if (chapterNumber < 100)
        return QStringLiteral("ch%1").arg(chapterNumber, 2, 10, QLatin1Char('0'));
    return QStringLiteral("ch%1").arg(chapterNumber);
}

QVariantMap EntityStore::chapterMeta(int chapterNumber) const
{
    const QString chaptersDir = m_bookDir + QStringLiteral("/chapters");
    const QString fileName = chapterFileName(chapterNumber) + QStringLiteral(".meta.json");
    const QString path = chaptersDir + QLatin1Char('/') + fileName;
    const QVariant v = readJsonFile(path);
    if (v.type() != QVariant::Map)
        return QVariantMap();
    return v.toMap();
}

void EntityStore::setChapterMeta(int chapterNumber, const QVariantMap &meta)
{
    const QString chaptersDir = m_bookDir + QStringLiteral("/chapters");
    QDir().mkpath(chaptersDir);
    const QString fileName = chapterFileName(chapterNumber) + QStringLiteral(".meta.json");
    const QString path = chaptersDir + QLatin1Char('/') + fileName;

    QVariantMap existing = chapterMeta(chapterNumber);
    for (auto it = meta.begin(); it != meta.end(); ++it) {
        existing[it.key()] = it.value();
    }
    writeJsonFile(path, existing);
}

QVariantList EntityStore::linkedEntities(int chapterNumber, const QString &linkKey, const QVariantList &allEntities) const
{
    const QVariantMap meta = chapterMeta(chapterNumber);
    const QVariantList ids = meta.value(linkKey).toList();

    QVariantList result;
    for (const QVariant &idVar : ids) {
        const int id = idVar.toInt();
        for (const QVariant &entity : allEntities) {
            const QVariantMap m = entity.toMap();
            if (m.value(QStringLiteral("id")).toInt() == id) {
                result.append(m);
                break;
            }
        }
    }
    return result;
}

QVariantList EntityStore::linkedCharacters(int chapterNumber) const
{
    return linkedEntities(chapterNumber, QStringLiteral("linkedCharacters"), characters());
}

QVariantList EntityStore::linkedTerms(int chapterNumber) const
{
    return linkedEntities(chapterNumber, QStringLiteral("linkedTerms"), terms());
}

QVariantList EntityStore::linkedKnowledge(int chapterNumber) const
{
    return linkedEntities(chapterNumber, QStringLiteral("linkedKnowledge"), knowledgeCards());
}

QVariantList EntityStore::linkedMemos(int chapterNumber) const
{
    return linkedEntities(chapterNumber, QStringLiteral("linkedMemos"), memos());
}

QVariantList EntityStore::linkedOutlines(int chapterNumber) const
{
    return linkedEntities(chapterNumber, QStringLiteral("linkedOutlines"), outlines());
}

// ---- Chapter for Prompt ----

QVariantMap EntityStore::chapterForPrompt(int chapterNumber) const
{
    QVariantMap result;

    const QVariantMap meta = chapterMeta(chapterNumber);
    result[QStringLiteral("title")] = meta.value(QStringLiteral("title")).toString();
    result[QStringLiteral("summary")] = meta.value(QStringLiteral("summary")).toString();

    const QString chaptersDir = m_bookDir + QStringLiteral("/chapters");
    const QString fileName = chapterFileName(chapterNumber) + QStringLiteral(".txt");
    const QString contentPath = chaptersDir + QLatin1Char('/') + fileName;

    QFile f(contentPath);
    bool hasContent = false;
    QString content;
    if (f.open(QIODevice::ReadOnly)) {
        QTextStream in(&f);
        content = in.readAll();
        hasContent = !content.isEmpty();
    }

    result[QStringLiteral("content")] = content;
    result[QStringLiteral("hasContent")] = hasContent;

    return result;
}

// ---- Chapter Inheritance ----

QVariantMap EntityStore::inheritedAiConfig(int fromChapterNumber) const
{
    if (fromChapterNumber <= 0)
        return QVariantMap();

    const QVariantMap meta = chapterMeta(fromChapterNumber);
    const QVariantMap aiConfig = meta.value(QStringLiteral("aiConfig")).toMap();
    if (aiConfig.isEmpty())
        return QVariantMap();

    QVariantMap result = aiConfig;
    result.remove(QStringLiteral("chapterPlot"));
    result.remove(QStringLiteral("styleTemplateId"));
    return result;
}
