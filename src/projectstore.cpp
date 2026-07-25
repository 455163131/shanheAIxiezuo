#include "projectstore.h"

#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUuid>
#include <QDateTime>
#include <QDebug>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>

namespace {
constexpr int SCHEMA_VERSION = 3;

QString chapterBaseName(int n)
{
    // 1 -> "ch01", 12 -> "ch12"
    return QStringLiteral("ch%1").arg(n, 2, 10, QLatin1Char('0'));
}

bool copyRecursively(const QString &src, const QString &dst)
{
    QDir srcDir(src);
    if (!srcDir.exists())
        return false;
    QDir().mkpath(dst);
    const QStringList files = srcDir.entryList(QDir::Files);
    for (const QString &fn : files) {
        if (!QFile::copy(src + QLatin1Char('/') + fn, dst + QLatin1Char('/') + fn))
            return false;
    }
    const QStringList dirs = srcDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    for (const QString &dn : dirs) {
        if (!copyRecursively(src + QLatin1Char('/') + dn, dst + QLatin1Char('/') + dn))
            return false;
    }
    return true;
}

} // namespace

ProjectStore::ProjectStore(QObject *parent)
    : QObject(parent)
{
    QDir root(rootPath());
    if (!root.exists())
        root.mkpath(QStringLiteral("."));
}

QString ProjectStore::rootPath() const
{
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
           + QStringLiteral("/books");
}

QString ProjectStore::bookDir(const QString &id) const
{
    return rootPath() + QLatin1Char('/') + id;
}

QString ProjectStore::metaPath(const QString &id) const
{
    return bookDir(id) + QStringLiteral("/meta.json");
}
QString ProjectStore::biblePath(const QString &id) const
{
    return bookDir(id) + QStringLiteral("/bible.md");
}
QString ProjectStore::charactersPath(const QString &id) const
{
    return bookDir(id) + QStringLiteral("/characters.json");
}
QString ProjectStore::outlinePath(const QString &id) const
{
    return bookDir(id) + QStringLiteral("/outline.json");
}
QString ProjectStore::summariesPath(const QString &id) const
{
    return bookDir(id) + QStringLiteral("/summaries.json");
}
QString ProjectStore::templatePath(const QString &id) const
{
    return bookDir(id) + QStringLiteral("/template.md");
}
QString ProjectStore::chaptersDir(const QString &id) const
{
    return bookDir(id) + QStringLiteral("/chapters");
}
QString ProjectStore::chapterPath(const QString &id, int n) const
{
    return chaptersDir(id) + QLatin1Char('/') + chapterBaseName(n) + QStringLiteral(".txt");
}

// ---- 文件辅助（直接 UTF-8 字节，规避 Qt 版本间 codec 差异）----
bool ProjectStore::writeTextFile(const QString &path, const QString &text)
{
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate))
        return false;
    return f.write(text.toUtf8()) >= 0;
}
QString ProjectStore::readTextFile(const QString &path)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly))
        return QString();
    return QString::fromUtf8(f.readAll());
}
bool ProjectStore::writeJsonFile(const QString &path, const QVariant &v)
{
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate))
        return false;
    const QJsonDocument doc = QJsonDocument::fromVariant(v);
    return f.write(doc.toJson(QJsonDocument::Indented)) >= 0;
}
QVariant ProjectStore::readJsonFile(const QString &path)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly))
        return QVariant();
    const QJsonDocument doc = QJsonDocument::fromJson(f.readAll());
    return doc.toVariant();
}

// ---- 写：把整张 book map 拆到目录布局 ----
void ProjectStore::writeBookDir(const QString &id, const QVariantMap &book) const
{
    const QString dir = bookDir(id);
    QDir(dir).mkpath(QStringLiteral("."));
    QDir(chaptersDir(id)).mkpath(QStringLiteral("."));

    // 元数据（超集：剔除正文类，正文类拆到独立文件）
    QVariantMap meta;
    meta[QStringLiteral("schemaVersion")] = SCHEMA_VERSION;
    meta[QStringLiteral("id")] = id;
    meta[QStringLiteral("title")] = book.value(QStringLiteral("title"));
    meta[QStringLiteral("genreId")] = book.value(QStringLiteral("genreId"));
    meta[QStringLiteral("genreName")] = book.value(QStringLiteral("genreName"));
    meta[QStringLiteral("author")] = book.value(QStringLiteral("author"));
    meta[QStringLiteral("hue")] = book.value(QStringLiteral("hue"));
    meta[QStringLiteral("timeline")] = book.value(QStringLiteral("timeline"));
    // 开新书「引导访谈」答案（增量字段，向后兼容：旧书 meta 无此 key 则加载为空）
    meta[QStringLiteral("direction")] = book.value(QStringLiteral("direction"));
    meta[QStringLiteral("tone")]      = book.value(QStringLiteral("tone"));
    meta[QStringLiteral("hook")]      = book.value(QStringLiteral("hook"));
    meta[QStringLiteral("createdAt")] = book.value(
        QStringLiteral("createdAt"), QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
    meta[QStringLiteral("updatedAt")] = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);

    // 章节标题单独保留（chNN.txt 只存正文，标题不入契约文本文件）
    QVariantList chaptersMeta;
    const QVariantList chapters = book.value(QStringLiteral("chapters")).toList();
    for (const QVariant &cv : chapters) {
        const QVariantMap cm = cv.toMap();
        chaptersMeta.append(cm.value(QStringLiteral("title"),
            QStringLiteral("未命名章")));
    }
    meta[QStringLiteral("chaptersMeta")] = chaptersMeta;
    writeJsonFile(metaPath(id), meta);

    // 拆出的内容文件
    writeTextFile(biblePath(id), book.value(QStringLiteral("worldView")).toString());
    writeJsonFile(charactersPath(id), QVariantMap{
        { QStringLiteral("raw"), book.value(QStringLiteral("characters")).toString() } });
    writeJsonFile(outlinePath(id), QVariantMap{
        { QStringLiteral("book"), book.value(QStringLiteral("outlineText")).toString() } });
    if (!QFile::exists(summariesPath(id)))
        writeJsonFile(summariesPath(id), QVariantMap());
    if (!QFile::exists(templatePath(id)))
        writeTextFile(templatePath(id), QString());

    // 章节正文（1-based 两位补零）
    int n = 0;
    for (const QVariant &cv : chapters) {
        const QVariantMap cm = cv.toMap();
        ++n;
        writeTextFile(chapterPath(id, n), cm.value(QStringLiteral("content")).toString());
    }
    cleanupStaleChapters(id, n);
}

void ProjectStore::cleanupStaleChapters(const QString &id, int keepCount) const
{
    QDir dir(chaptersDir(id));
    const QStringList files = dir.entryList({ QStringLiteral("ch*.txt") }, QDir::Files);
    for (const QString &fn : files) {
        const int dot = fn.indexOf(QLatin1Char('.'));
        const int num = fn.mid(2, dot - 2).toInt();
        if (num > keepCount)
            dir.remove(fn);
    }
}

// ---- 读：把目录布局合并回整张 book map ----
QVariantMap ProjectStore::readBookDir(const QString &id) const
{
    const QVariantMap meta = readJsonFile(metaPath(id)).toMap();
    QVariantMap book;
    book[QStringLiteral("id")] = id;
    book[QStringLiteral("title")] = meta.value(QStringLiteral("title"));
    book[QStringLiteral("genreId")] = meta.value(QStringLiteral("genreId"));
    book[QStringLiteral("genreName")] = meta.value(QStringLiteral("genreName"));
    book[QStringLiteral("author")] = meta.value(QStringLiteral("author"));
    book[QStringLiteral("hue")] = meta.value(QStringLiteral("hue"));
    book[QStringLiteral("timeline")] = meta.value(QStringLiteral("timeline"));
    // 引导访谈答案回读（Studio 写第一章时复用 hook/tone 提升贴合度；缺则空，优雅降级）
    book[QStringLiteral("direction")] = meta.value(QStringLiteral("direction"));
    book[QStringLiteral("tone")]      = meta.value(QStringLiteral("tone"));
    book[QStringLiteral("hook")]      = meta.value(QStringLiteral("hook"));

    book[QStringLiteral("worldView")] = readTextFile(biblePath(id));
    book[QStringLiteral("characters")] =
        readJsonFile(charactersPath(id)).toMap().value(QStringLiteral("raw")).toString();
    book[QStringLiteral("outlineText")] =
        readJsonFile(outlinePath(id)).toMap().value(QStringLiteral("book")).toString();

    const QVariantList chaptersMeta = meta.value(QStringLiteral("chaptersMeta")).toList();
    QVariantList chapters;
    QDir dir(chaptersDir(id));
    const QStringList files = dir.entryList({ QStringLiteral("ch*.txt") }, QDir::Files, QDir::Name);
    int i = 0;
    for (const QString &fn : files) {
        const int dot = fn.indexOf(QLatin1Char('.'));
        const int num = fn.mid(2, dot - 2).toInt();
        QVariantMap cm;
        cm[QStringLiteral("title")] = (i < chaptersMeta.size())
            ? chaptersMeta.at(i).toString()
            : QStringLiteral("第%1章").arg(num);
        cm[QStringLiteral("content")] = readTextFile(chapterPath(id, num));
        chapters.append(cm);
        ++i;
    }
    book[QStringLiteral("chapters")] = chapters;
    return book;
}


// ---- v2 -> v3 migration: meta prefs, chapter meta files, entity stores ----
bool ProjectStore::migrateV2toV3(const QString &id) const
{
    const QString dir = bookDir(id);
    const QString backupDir = dir + QStringLiteral(".backup_v2");

    // Step 1: Backup entire book directory
    if (QDir(backupDir).exists())
        QDir(backupDir).removeRecursively();
    if (!QDir().mkpath(backupDir))
        return false;

    if (!copyRecursively(dir, backupDir)) {
        QDir(backupDir).removeRecursively();
        return false;
    }

    bool success = true;

    try {
        // Step 2: Upgrade meta.json: schemaVersion 2->3, add prefs defaults
        const QString mp = metaPath(id);
        QVariantMap meta = readJsonFile(mp).toMap();
        meta[QStringLiteral("schemaVersion")] = 3;

        QVariantMap prefs;
        prefs[QStringLiteral("creativityIndex")] = 3;
        prefs[QStringLiteral("thinkingAuto")] = true;
        prefs[QStringLiteral("thinkingIndex")] = 2;
        prefs[QStringLiteral("wordCountMin")] = 2000;
        prefs[QStringLiteral("wordCountMax")] = 2500;
        prefs[QStringLiteral("recentMode")] = QStringLiteral("lastN");
        prefs[QStringLiteral("recentValue")] = 2000;
        meta[QStringLiteral("prefs")] = prefs;

        if (!writeJsonFile(mp, meta))
            throw QString("Failed to write meta.json");

        // Step 3: Generate chNN.meta.json from chaptersMeta
        const QVariantList chaptersMeta = meta.value(QStringLiteral("chaptersMeta")).toList();
        const QString chDir = chaptersDir(id);
        for (int i = 0; i < chaptersMeta.size(); ++i) {
            const int chapNum = i + 1;
            const QString chBase = chapterBaseName(chapNum);
            const QString chMetaPath = chDir + QLatin1Char('/') + chBase + QStringLiteral(".meta.json");
            if (!QFile::exists(chMetaPath)) {
                QVariantMap chMeta;
                chMeta[QStringLiteral("title")] = chaptersMeta.at(i).toString();
                chMeta[QStringLiteral("aiConfig")] = QVariantMap();
                if (!writeJsonFile(chMetaPath, chMeta))
                    throw QString("Failed to write chapter meta for ch%1").arg(chapNum);
            }
        }

        // Step 4: Upgrade characters.json: raw -> items[0].description
        const QString cp = charactersPath(id);
        QVariantMap chars = readJsonFile(cp).toMap();
        const QString rawContent = chars.value(QStringLiteral("raw")).toString();
        QVariantList items;
        if (!rawContent.isEmpty()) {
            QVariantMap firstItem;
            firstItem[QStringLiteral("id")] = 1;
            firstItem[QStringLiteral("name")] = QStringLiteral("导入角色");
            firstItem[QStringLiteral("description")] = rawContent;
            items.append(firstItem);
        }
        chars[QStringLiteral("schemaVersion")] = 1;
        chars[QStringLiteral("items")] = items;
        if (!chars.contains(QStringLiteral("folders")))
            chars[QStringLiteral("folders")] = QVariantList();
        if (!writeJsonFile(cp, chars))
            throw QString("Failed to write characters.json");

        // Step 5: Create empty entity files
        struct EntityFile {
            const char *name;
            bool hasFolders;
        };
        const EntityFile entities[] = {
            { "terms.json", false },
            { "knowledge.json", false },
            { "memos.json", false },
            { "outlines.json", false },
        };
        for (const auto &e : entities) {
            const QString path = dir + QLatin1Char('/') + QString::fromUtf8(e.name);
            if (!QFile::exists(path)) {
                QVariantMap root;
                root[QStringLiteral("schemaVersion")] = 1;
                root[QStringLiteral("items")] = QVariantList();
                if (e.hasFolders)
                    root[QStringLiteral("folders")] = QVariantList();
                if (!writeJsonFile(path, root))
                    throw QString("Failed to write %1").arg(QString::fromUtf8(e.name));
            }
        }

    } catch (const QString &err) {
        qWarning() << "v2->v3 migration failed:" << err;
        success = false;
    }

    if (success) {
        QDir(backupDir).removeRecursively();
    } else {
        QDir(dir).removeRecursively();
        QDir().mkpath(dir);
        copyRecursively(backupDir, dir);
        QDir(backupDir).removeRecursively();
    }

    return success;
}


// ---- 向后兼容：旧版单一 meta.json 自动拆出 ----
void ProjectStore::migrateIfNeeded(const QString &id) const
{
    const QString mp = metaPath(id);
    if (!QFile::exists(mp))
        return; // 新书或从未写，无需迁移
    const QVariantMap m = readJsonFile(mp).toMap();
    // 旧版标志：无 schemaVersion 且含内联 chapters 数组
    const bool isLegacy = !m.contains(QStringLiteral("schemaVersion"))
                          && m.contains(QStringLiteral("chapters"));
    if (isLegacy) {
        // 旧版整张 map 已是完整 book 结构（worldView/characters/timeline/outlineText/chapters）
        // 直接交给 writeBookDir 重写为新布局（章节标题经 chaptersMeta 保留）。
        writeBookDir(id, m);
        return;
    }

    // v2 -> v3 migration
    const int schemaVersion = m.value(QStringLiteral("schemaVersion")).toInt();
    if (schemaVersion == 2) {
        migrateV2toV3(id);
    }
}

bool ProjectStore::exists(const QString &id) const
{
    return QFile::exists(bookDir(id));
}

QString ProjectStore::createBook(const QVariantMap &book)
{
    const QString id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    writeBookDir(id, book);
    return id;
}

QVariantMap ProjectStore::loadBook(const QString &id) const
{
    migrateIfNeeded(id);
    return readBookDir(id);
}

bool ProjectStore::saveBook(const QVariantMap &book) const
{
    const QString id = book.value(QStringLiteral("id")).toString();
    if (id.isEmpty() || !exists(id))
        return false;
    writeBookDir(id, book);
    return true;
}

QVariantList ProjectStore::listBooks() const
{
    QVariantList result;
    QDir root(rootPath());
    if (!root.exists())
        return result;
    const QStringList dirs = root.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    for (const QString &d : dirs) {
        const QVariantMap m = readJsonFile(metaPath(d)).toMap();
        if (m.isEmpty())
            continue;
        QVariantMap card;
        card[QStringLiteral("id")] = d;
        card[QStringLiteral("title")] = m.value(QStringLiteral("title"));
        card[QStringLiteral("genreId")] = m.value(QStringLiteral("genreId"));
        card[QStringLiteral("genreName")] = m.value(QStringLiteral("genreName"));
        card[QStringLiteral("author")] = m.value(QStringLiteral("author"));
        card[QStringLiteral("hue")] = m.value(QStringLiteral("hue"));
        result.append(card);
    }
    return result;
}

QString ProjectStore::lastBookId() const
{
    QFile f(rootPath() + QStringLiteral("/.last"));
    if (!f.open(QIODevice::ReadOnly))
        return {};
    const QString id = QString::fromUtf8(f.readAll()).trimmed();
    return exists(id) ? id : QString();
}

void ProjectStore::setLastBookId(const QString &id)
{
    QDir(rootPath()).mkpath(QStringLiteral("."));
    QFile f(rootPath() + QStringLiteral("/.last"));
    if (f.open(QIODevice::WriteOnly | QIODevice::Truncate))
        f.write(id.toUtf8());
}


// ---- Import from project1 ai_writing.db (SQLite) ----
bool ProjectStore::importFromAiWritingDb(const QString &dbPath, const QString &booksDir)
{
    if (!QSqlDatabase::isDriverAvailable("QSQLITE")) {
        qWarning() << "QSQLITE driver not available, cannot import";
        return false;
    }

    if (!QFile::exists(dbPath)) {
        qWarning() << "Database file not found:" << dbPath;
        return false;
    }

    QDir().mkpath(booksDir);

    const QString connName = QStringLiteral("ai_writing_import");
    {
        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", connName);
        db.setDatabaseName(dbPath);
        if (!db.open()) {
            qWarning() << "Failed to open database:" << db.lastError().text();
            QSqlDatabase::removeDatabase(connName);
            return false;
        }

        // Read first novel
        QSqlQuery novelQ(db);
        novelQ.exec("SELECT id, title, style FROM novels ORDER BY id LIMIT 1");
        if (!novelQ.next()) {
            qWarning() << "No novels found in database";
            db.close();
            QSqlDatabase::removeDatabase(connName);
            return false;
        }

        int novelId = novelQ.value(0).toInt();
        QString title = novelQ.value(1).toString();
        QString style = novelQ.value(2).toString();

        // Generate new book id
        QString bookId = QUuid::createUuid().toString(QUuid::WithoutBraces);
        QString bookDir = booksDir + QLatin1Char('/') + bookId;
        QString chaptersDir = bookDir + QStringLiteral("/chapters");
        QDir().mkpath(bookDir);
        QDir().mkpath(chaptersDir);

        // Default prefs
        QVariantMap prefs;
        prefs[QStringLiteral("creativityIndex")] = 3;
        prefs[QStringLiteral("thinkingAuto")] = true;
        prefs[QStringLiteral("thinkingIndex")] = 2;
        prefs[QStringLiteral("wordCountMin")] = 2000;
        prefs[QStringLiteral("wordCountMax")] = 2500;
        prefs[QStringLiteral("recentMode")] = QStringLiteral("lastN");
        prefs[QStringLiteral("recentValue")] = 2000;

        // Read chapters
        QVariantList chaptersMeta;
        QSqlQuery chapQ(db);
        chapQ.prepare("SELECT title, content, summary, sort_order, word_count FROM chapters WHERE novel_id = :novelId ORDER BY sort_order");
        chapQ.bindValue(":novelId", novelId);
        chapQ.exec();

        int chapNum = 0;
        while (chapQ.next()) {
            ++chapNum;
            QString chTitle = chapQ.value(0).toString();
            QString chContent = chapQ.value(1).toString();
            QString chSummary = chapQ.value(2).toString();
            int wordCount = chapQ.value(4).toInt();

            QString chBase = chapterBaseName(chapNum);

            // Write chapter text
            writeTextFile(chaptersDir + QLatin1Char('/') + chBase + QStringLiteral(".txt"), chContent);

            // Write chapter meta
            QVariantMap chMeta;
            chMeta[QStringLiteral("title")] = chTitle;
            chMeta[QStringLiteral("summary")] = chSummary;
            chMeta[QStringLiteral("wordCount")] = wordCount;
            chMeta[QStringLiteral("aiConfig")] = QVariantMap();
            writeJsonFile(chaptersDir + QLatin1Char('/') + chBase + QStringLiteral(".meta.json"), chMeta);

            chaptersMeta.append(chTitle);
        }

        // Write meta.json
        QVariantMap meta;
        meta[QStringLiteral("schemaVersion")] = SCHEMA_VERSION;
        meta[QStringLiteral("id")] = bookId;
        meta[QStringLiteral("title")] = title;
        meta[QStringLiteral("chaptersMeta")] = chaptersMeta;
        meta[QStringLiteral("prefs")] = prefs;
        meta[QStringLiteral("createdAt")] = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
        meta[QStringLiteral("updatedAt")] = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
        writeJsonFile(bookDir + QStringLiteral("/meta.json"), meta);

        // Write bible.md (empty for now)
        writeTextFile(bookDir + QStringLiteral("/bible.md"), QString());

        // Read characters and character_folders
        QVariantList charItems;
        QSqlQuery charQ(db);
        charQ.prepare("SELECT id, name, description, folder_id, sort_order, pinned, hidden FROM characters WHERE novel_id = :novelId ORDER BY sort_order");
        charQ.bindValue(":novelId", novelId);
        charQ.exec();
        while (charQ.next()) {
            QVariantMap item;
            item[QStringLiteral("id")] = charQ.value(0).toInt();
            item[QStringLiteral("name")] = charQ.value(1).toString();
            item[QStringLiteral("description")] = charQ.value(2).toString();
            item[QStringLiteral("folderId")] = charQ.value(3).toInt();
            item[QStringLiteral("sortOrder")] = charQ.value(4).toInt();
            item[QStringLiteral("pinned")] = charQ.value(5).toBool();
            item[QStringLiteral("hidden")] = charQ.value(6).toBool();
            charItems.append(item);
        }

        QVariantList charFolders;
        QSqlQuery charFolderQ(db);
        charFolderQ.prepare("SELECT id, name, sort_order FROM character_folders WHERE novel_id = :novelId ORDER BY sort_order");
        charFolderQ.bindValue(":novelId", novelId);
        charFolderQ.exec();
        while (charFolderQ.next()) {
            QVariantMap folder;
            folder[QStringLiteral("id")] = charFolderQ.value(0).toInt();
            folder[QStringLiteral("name")] = charFolderQ.value(1).toString();
            folder[QStringLiteral("sortOrder")] = charFolderQ.value(2).toInt();
            charFolders.append(folder);
        }

        QVariantMap charsRoot;
        charsRoot[QStringLiteral("schemaVersion")] = 1;
        charsRoot[QStringLiteral("items")] = charItems;
        charsRoot[QStringLiteral("folders")] = charFolders;
        writeJsonFile(bookDir + QStringLiteral("/characters.json"), charsRoot);

        // Helper function to import simple entity tables
        auto importSimpleEntity = [&](const QString &tableName, const QString &fileName,
                                       const QString &idCol, const QStringList &fieldMap) {
            QVariantList items;
            QSqlQuery q(db);
            QString queryStr = QString("SELECT %1").arg(idCol);
            for (const QString &field : fieldMap) {
                queryStr += ", " + field;
            }
            queryStr += QString(" FROM %1 WHERE novel_id = :novelId").arg(tableName);
            q.prepare(queryStr);
            q.bindValue(":novelId", novelId);
            q.exec();
            while (q.next()) {
                QVariantMap item;
                item[QStringLiteral("id")] = q.value(0).toInt();
                for (int i = 0; i < fieldMap.size(); ++i) {
                    item[fieldMap[i]] = q.value(i + 1);
                }
                items.append(item);
            }
            QVariantMap root;
            root[QStringLiteral("schemaVersion")] = 1;
            root[QStringLiteral("items")] = items;
            writeJsonFile(bookDir + QLatin1Char('/') + fileName, root);
        };

        // Import terms
        importSimpleEntity("terms", "terms.json", "id",
                          QStringList() << "name" << "content" << "category");

        // Import knowledge_cards
        QVariantList knowledgeItems;
        QVariantList globalKnowledgeItems;
        QSqlQuery kq(db);
        kq.prepare("SELECT id, title, content, category, is_global FROM knowledge_cards WHERE novel_id = :novelId");
        kq.bindValue(":novelId", novelId);
        kq.exec();
        while (kq.next()) {
            QVariantMap item;
            item[QStringLiteral("id")] = kq.value(0).toInt();
            item[QStringLiteral("title")] = kq.value(1).toString();
            item[QStringLiteral("content")] = kq.value(2).toString();
            item[QStringLiteral("category")] = kq.value(3).toString();
            item[QStringLiteral("isGlobal")] = kq.value(4).toInt() == 1;
            knowledgeItems.append(item);
            if (kq.value(4).toInt() == 1) {
                globalKnowledgeItems.append(item);
            }
        }
        QVariantMap knowledgeRoot;
        knowledgeRoot[QStringLiteral("schemaVersion")] = 1;
        knowledgeRoot[QStringLiteral("items")] = knowledgeItems;
        writeJsonFile(bookDir + QStringLiteral("/knowledge.json"), knowledgeRoot);

        // Write global knowledge if any (at booksDir level)
        if (!globalKnowledgeItems.isEmpty()) {
            QString globalKPath = booksDir + QStringLiteral("/global_knowledge.json");
            QVariantMap gRoot;
            if (QFile::exists(globalKPath)) {
                gRoot = readJsonFile(globalKPath).toMap();
            }
            if (!gRoot.contains("items")) {
                gRoot[QStringLiteral("schemaVersion")] = 1;
                gRoot[QStringLiteral("items")] = QVariantList();
            }
            QVariantList existing = gRoot["items"].toList();
            for (const QVariant &item : globalKnowledgeItems) {
                existing.append(item);
            }
            gRoot[QStringLiteral("items")] = existing;
            writeJsonFile(globalKPath, gRoot);
        }

        // Import memos
        importSimpleEntity("memos", "memos.json", "id",
                          QStringList() << "title" << "content");

        // Import outlines
        importSimpleEntity("outlines", "outlines.json", "id",
                          QStringList() << "title" << "content" << "type");

        // Import templates (merge to global templates.json at booksDir level)
        QSqlQuery tplQ(db);
        tplQ.exec("SELECT type, title, content FROM templates");
        QString tplPath = booksDir + QStringLiteral("/templates.json");
        QVariantMap tplRoot;
        if (QFile::exists(tplPath)) {
            tplRoot = readJsonFile(tplPath).toMap();
        }
        if (!tplRoot.contains("items")) {
            tplRoot[QStringLiteral("schemaVersion")] = 1;
            tplRoot[QStringLiteral("items")] = QVariantList();
        }
        QVariantList tplItems = tplRoot["items"].toList();
        QSet<QString> existingTitles;
        for (const QVariant &item : tplItems) {
            existingTitles.insert(item.toMap()["title"].toString());
        }
        int nextTplId = tplItems.size() + 1;
        while (tplQ.next()) {
            QString tplTitle = tplQ.value(1).toString();
            if (!existingTitles.contains(tplTitle)) {
                QVariantMap item;
                item[QStringLiteral("id")] = nextTplId++;
                item[QStringLiteral("type")] = tplQ.value(0).toString();
                item[QStringLiteral("title")] = tplTitle;
                item[QStringLiteral("content")] = tplQ.value(2).toString();
                tplItems.append(item);
                existingTitles.insert(tplTitle);
            }
        }
        tplRoot[QStringLiteral("items")] = tplItems;
        writeJsonFile(tplPath, tplRoot);

        // Write other required files
        writeJsonFile(bookDir + QStringLiteral("/outline.json"),
                     QVariantMap{{QStringLiteral("book"), QString()}});
        writeJsonFile(bookDir + QStringLiteral("/summaries.json"), QVariantMap());
        writeTextFile(bookDir + QStringLiteral("/template.md"), style);

        db.close();
    }

    QSqlDatabase::removeDatabase(connName);
    return true;
}
