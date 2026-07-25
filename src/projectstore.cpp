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
