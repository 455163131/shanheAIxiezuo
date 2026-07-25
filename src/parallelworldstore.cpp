#include "parallelworldstore.h"

#include <QCoreApplication>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDateTime>
#include <QDebug>

namespace {
constexpr int SCHEMA_VERSION = 1;
constexpr int MAX_JSON_BRANCHES = 500;
const char *CONNECTION_NAME = "parallel_world_store";

QString dbPath()
{
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
           + QStringLiteral("/parallel_worlds.db");
}

qint64 nowMs()
{
    return QDateTime::currentMSecsSinceEpoch();
}

ParallelBranch branchFromSqlQuery(QSqlQuery &q)
{
    ParallelBranch b;
    b.id = q.value(0).toLongLong();
    b.novelId = q.value(1).toString();
    b.chapterId = q.value(2).toString();
    b.parentId = q.value(3).toLongLong();
    b.branchName = q.value(4).toString();
    b.baseVersion = q.value(5).toString();
    b.contentSnapshot = q.value(6).toString();
    b.currentContent = q.value(7).toString();
    b.status = q.value(8).toString();
    b.createdAt = q.value(9).toLongLong();
    b.updatedAt = q.value(10).toLongLong();
    b.wordCount = q.value(11).toInt();
    return b;
}

QVariantMap branchToVariantMap(const ParallelBranch &b)
{
    QVariantMap m;
    m[QStringLiteral("id")] = b.id;
    m[QStringLiteral("novelId")] = b.novelId;
    m[QStringLiteral("chapterId")] = b.chapterId;
    m[QStringLiteral("parentId")] = b.parentId;
    m[QStringLiteral("branchName")] = b.branchName;
    m[QStringLiteral("baseVersion")] = b.baseVersion;
    m[QStringLiteral("contentSnapshot")] = b.contentSnapshot;
    m[QStringLiteral("currentContent")] = b.currentContent;
    m[QStringLiteral("status")] = b.status;
    m[QStringLiteral("createdAt")] = b.createdAt;
    m[QStringLiteral("updatedAt")] = b.updatedAt;
    m[QStringLiteral("wordCount")] = b.wordCount;
    return m;
}

ParallelBranch branchFromVariantMap(const QVariantMap &m)
{
    ParallelBranch b;
    b.id = m.value(QStringLiteral("id")).toLongLong();
    b.novelId = m.value(QStringLiteral("novelId")).toString();
    b.chapterId = m.value(QStringLiteral("chapterId")).toString();
    b.parentId = m.value(QStringLiteral("parentId")).toLongLong();
    b.branchName = m.value(QStringLiteral("branchName")).toString();
    b.baseVersion = m.value(QStringLiteral("baseVersion")).toString();
    b.contentSnapshot = m.value(QStringLiteral("contentSnapshot")).toString();
    b.currentContent = m.value(QStringLiteral("currentContent")).toString();
    b.status = m.value(QStringLiteral("status")).toString();
    b.createdAt = m.value(QStringLiteral("createdAt")).toLongLong();
    b.updatedAt = m.value(QStringLiteral("updatedAt")).toLongLong();
    b.wordCount = m.value(QStringLiteral("wordCount")).toInt();
    return b;
}
} // namespace

ParallelWorldStore &ParallelWorldStore::instance()
{
    static ParallelWorldStore s_instance;
    return s_instance;
}

ParallelWorldStore::ParallelWorldStore()
    : m_hasSqlite(false)
{
    initSqlite();
    if (!m_hasSqlite) {
        m_jsonPath = jsonPath();
        loadJson();
    }
}

bool ParallelWorldStore::hasSqlite() const
{
    return m_hasSqlite;
}

QString ParallelWorldStore::storageMode() const
{
    return m_hasSqlite ? QStringLiteral("sqlite") : QStringLiteral("json");
}

void ParallelWorldStore::initSqlite()
{
    if (!QSqlDatabase::isDriverAvailable(QStringLiteral("QSQLITE")))
        return;

    QDir().mkpath(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation));

    QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"),
                                                 QLatin1String(CONNECTION_NAME));
    db.setDatabaseName(dbPath());
    if (!db.open()) {
        qWarning() << "ParallelWorldStore: SQLite open failed:" << db.lastError().text();
        QSqlDatabase::removeDatabase(QLatin1String(CONNECTION_NAME));
        return;
    }

    QSqlQuery q(db);
    const QString createTable = QStringLiteral(
        "CREATE TABLE IF NOT EXISTS parallel_worlds ("
        "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  novelId TEXT NOT NULL,"
        "  chapterId TEXT NOT NULL,"
        "  parentId INTEGER,"
        "  branchName TEXT,"
        "  baseVersion TEXT NOT NULL,"
        "  contentSnapshot TEXT NOT NULL,"
        "  currentContent TEXT,"
        "  status TEXT NOT NULL DEFAULT 'active',"
        "  createdAt INTEGER NOT NULL,"
        "  updatedAt INTEGER NOT NULL,"
        "  wordCount INTEGER DEFAULT 0"
        ")");
    if (!q.exec(createTable)) {
        qWarning() << "ParallelWorldStore: create table failed:" << q.lastError().text();
        db.close();
        QSqlDatabase::removeDatabase(QLatin1String(CONNECTION_NAME));
        return;
    }

    q.exec(QStringLiteral(
        "CREATE INDEX IF NOT EXISTS idx_pw_novel_chapter "
        "ON parallel_worlds(novelId, chapterId)"));
    q.exec(QStringLiteral(
        "CREATE INDEX IF NOT EXISTS idx_pw_status "
        "ON parallel_worlds(status)"));

    m_hasSqlite = true;
}

QString ParallelWorldStore::jsonPath()
{
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
           + QStringLiteral("/parallel_worlds.json");
}

void ParallelWorldStore::loadJson()
{
    QFile f(m_jsonPath);
    if (!f.open(QIODevice::ReadOnly))
        return;
    const QJsonDocument doc = QJsonDocument::fromJson(f.readAll());
    const QVariantMap root = doc.toVariant().toMap();
    const QVariantList list = root.value(QStringLiteral("branches")).toList();
    m_jsonBranches.clear();
    for (const QVariant &v : list)
        m_jsonBranches.append(branchFromVariantMap(v.toMap()));
}

void ParallelWorldStore::saveJson()
{
    QDir().mkpath(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation));
    QVariantList list;
    for (const ParallelBranch &b : m_jsonBranches)
        list.append(branchToVariantMap(b));
    QVariantMap root;
    root[QStringLiteral("schemaVersion")] = SCHEMA_VERSION;
    root[QStringLiteral("branches")] = list;
    QFile f(m_jsonPath);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate))
        return;
    const QJsonDocument doc = QJsonDocument::fromVariant(root);
    f.write(doc.toJson(QJsonDocument::Indented));
}

qlonglong ParallelWorldStore::createBranch(const QString &novelId, const QString &chapterId,
                                           qlonglong parentId, const QString &branchName,
                                           const QString &contentSnapshot, int wordCount)
{
    const qint64 now = nowMs();
    const QString baseVersion = parentId == 0 ? QStringLiteral("main")
                                               : QString::number(parentId);

    if (m_hasSqlite) {
        QSqlDatabase db = QSqlDatabase::database(QLatin1String(CONNECTION_NAME));
        QSqlQuery q(db);
        q.prepare(QStringLiteral(
            "INSERT INTO parallel_worlds "
            "(novelId, chapterId, parentId, branchName, baseVersion, "
            " contentSnapshot, currentContent, status, createdAt, updatedAt, wordCount) "
            "VALUES (?, ?, ?, ?, ?, ?, ?, 'active', ?, ?, ?)"));
        q.addBindValue(novelId);
        q.addBindValue(chapterId);
        if (parentId == 0)
            q.addBindValue(QVariant(QVariant::LongLong));
        else
            q.addBindValue(parentId);
        q.addBindValue(branchName);
        q.addBindValue(baseVersion);
        q.addBindValue(contentSnapshot);
        q.addBindValue(contentSnapshot);
        q.addBindValue(now);
        q.addBindValue(now);
        q.addBindValue(wordCount);
        if (!q.exec()) {
            qWarning() << "ParallelWorldStore createBranch:" << q.lastError().text();
            return -1;
        }
        return q.lastInsertId().toLongLong();
    }

    qlonglong newId = 1;
    if (!m_jsonBranches.isEmpty()) {
        qlonglong maxId = 0;
        for (const ParallelBranch &b : m_jsonBranches) {
            if (b.id > maxId)
                maxId = b.id;
        }
        newId = maxId + 1;
    }

    ParallelBranch b;
    b.id = newId;
    b.novelId = novelId;
    b.chapterId = chapterId;
    b.parentId = parentId;
    b.branchName = branchName;
    b.baseVersion = baseVersion;
    b.contentSnapshot = contentSnapshot;
    b.currentContent = contentSnapshot;
    b.status = QStringLiteral("active");
    b.createdAt = now;
    b.updatedAt = now;
    b.wordCount = wordCount;

    m_jsonBranches.append(b);

    while (m_jsonBranches.size() > MAX_JSON_BRANCHES)
        m_jsonBranches.removeFirst();

    saveJson();
    return newId;
}

QList<ParallelBranch> ParallelWorldStore::listBranches(const QString &novelId, const QString &chapterId,
                                                       const QString &statusFilter)
{
    QList<ParallelBranch> result;

    if (m_hasSqlite) {
        QSqlDatabase db = QSqlDatabase::database(QLatin1String(CONNECTION_NAME));
        QSqlQuery q(db);
        QString sql = QStringLiteral(
            "SELECT id, novelId, chapterId, parentId, branchName, baseVersion, "
            "       contentSnapshot, currentContent, status, createdAt, updatedAt, wordCount "
            "FROM parallel_worlds "
            "WHERE novelId = ? AND chapterId = ?");
        if (!statusFilter.isEmpty())
            sql += QStringLiteral(" AND status = ?");
        sql += QStringLiteral(" ORDER BY id DESC");
        q.prepare(sql);
        q.addBindValue(novelId);
        q.addBindValue(chapterId);
        if (!statusFilter.isEmpty())
            q.addBindValue(statusFilter);
        if (q.exec()) {
            while (q.next())
                result.append(branchFromSqlQuery(q));
        }
        return result;
    }

    for (int i = m_jsonBranches.size() - 1; i >= 0; --i) {
        const ParallelBranch &b = m_jsonBranches.at(i);
        if (b.novelId == novelId && b.chapterId == chapterId) {
            if (statusFilter.isEmpty() || b.status == statusFilter)
                result.append(b);
        }
    }
    return result;
}

ParallelBranch ParallelWorldStore::getBranch(qlonglong id)
{
    ParallelBranch empty = {};
    empty.id = -1;

    if (m_hasSqlite) {
        QSqlDatabase db = QSqlDatabase::database(QLatin1String(CONNECTION_NAME));
        QSqlQuery q(db);
        q.prepare(QStringLiteral(
            "SELECT id, novelId, chapterId, parentId, branchName, baseVersion, "
            "       contentSnapshot, currentContent, status, createdAt, updatedAt, wordCount "
            "FROM parallel_worlds WHERE id = ?"));
        q.addBindValue(id);
        if (q.exec() && q.next())
            return branchFromSqlQuery(q);
        return empty;
    }

    for (const ParallelBranch &b : m_jsonBranches) {
        if (b.id == id)
            return b;
    }
    return empty;
}

bool ParallelWorldStore::updateContent(qlonglong id, const QString &content, int wordCount)
{
    const qint64 now = nowMs();

    if (m_hasSqlite) {
        QSqlDatabase db = QSqlDatabase::database(QLatin1String(CONNECTION_NAME));
        QSqlQuery q(db);
        q.prepare(QStringLiteral(
            "UPDATE parallel_worlds SET currentContent = ?, wordCount = ?, updatedAt = ? "
            "WHERE id = ?"));
        q.addBindValue(content);
        q.addBindValue(wordCount);
        q.addBindValue(now);
        q.addBindValue(id);
        return q.exec() && q.numRowsAffected() > 0;
    }

    for (int i = 0; i < m_jsonBranches.size(); ++i) {
        if (m_jsonBranches[i].id == id) {
            m_jsonBranches[i].currentContent = content;
            m_jsonBranches[i].wordCount = wordCount;
            m_jsonBranches[i].updatedAt = now;
            saveJson();
            return true;
        }
    }
    return false;
}

bool ParallelWorldStore::renameBranch(qlonglong id, const QString &newName)
{
    const qint64 now = nowMs();

    if (m_hasSqlite) {
        QSqlDatabase db = QSqlDatabase::database(QLatin1String(CONNECTION_NAME));
        QSqlQuery q(db);
        q.prepare(QStringLiteral(
            "UPDATE parallel_worlds SET branchName = ?, updatedAt = ? WHERE id = ?"));
        q.addBindValue(newName);
        q.addBindValue(now);
        q.addBindValue(id);
        return q.exec() && q.numRowsAffected() > 0;
    }

    for (int i = 0; i < m_jsonBranches.size(); ++i) {
        if (m_jsonBranches[i].id == id) {
            m_jsonBranches[i].branchName = newName;
            m_jsonBranches[i].updatedAt = now;
            saveJson();
            return true;
        }
    }
    return false;
}

bool ParallelWorldStore::markMerged(qlonglong id)
{
    const qint64 now = nowMs();

    if (m_hasSqlite) {
        QSqlDatabase db = QSqlDatabase::database(QLatin1String(CONNECTION_NAME));
        QSqlQuery q(db);
        q.prepare(QStringLiteral(
            "UPDATE parallel_worlds SET status = 'merged', updatedAt = ? WHERE id = ?"));
        q.addBindValue(now);
        q.addBindValue(id);
        return q.exec() && q.numRowsAffected() > 0;
    }

    for (int i = 0; i < m_jsonBranches.size(); ++i) {
        if (m_jsonBranches[i].id == id) {
            m_jsonBranches[i].status = QStringLiteral("merged");
            m_jsonBranches[i].updatedAt = now;
            saveJson();
            return true;
        }
    }
    return false;
}

bool ParallelWorldStore::markAbandoned(qlonglong id)
{
    const qint64 now = nowMs();

    if (m_hasSqlite) {
        QSqlDatabase db = QSqlDatabase::database(QLatin1String(CONNECTION_NAME));
        QSqlQuery q(db);
        q.prepare(QStringLiteral(
            "UPDATE parallel_worlds SET status = 'abandoned', updatedAt = ? WHERE id = ?"));
        q.addBindValue(now);
        q.addBindValue(id);
        return q.exec() && q.numRowsAffected() > 0;
    }

    for (int i = 0; i < m_jsonBranches.size(); ++i) {
        if (m_jsonBranches[i].id == id) {
            m_jsonBranches[i].status = QStringLiteral("abandoned");
            m_jsonBranches[i].updatedAt = now;
            saveJson();
            return true;
        }
    }
    return false;
}

bool ParallelWorldStore::reactivate(qlonglong id)
{
    const qint64 now = nowMs();

    if (m_hasSqlite) {
        QSqlDatabase db = QSqlDatabase::database(QLatin1String(CONNECTION_NAME));
        QSqlQuery q(db);
        q.prepare(QStringLiteral(
            "UPDATE parallel_worlds SET status = 'active', updatedAt = ? WHERE id = ?"));
        q.addBindValue(now);
        q.addBindValue(id);
        return q.exec() && q.numRowsAffected() > 0;
    }

    for (int i = 0; i < m_jsonBranches.size(); ++i) {
        if (m_jsonBranches[i].id == id) {
            m_jsonBranches[i].status = QStringLiteral("active");
            m_jsonBranches[i].updatedAt = now;
            saveJson();
            return true;
        }
    }
    return false;
}

bool ParallelWorldStore::deleteBranch(qlonglong id)
{
    if (m_hasSqlite) {
        QSqlDatabase db = QSqlDatabase::database(QLatin1String(CONNECTION_NAME));
        QSqlQuery q(db);
        q.prepare(QStringLiteral("DELETE FROM parallel_worlds WHERE id = ?"));
        q.addBindValue(id);
        return q.exec() && q.numRowsAffected() > 0;
    }

    for (int i = 0; i < m_jsonBranches.size(); ++i) {
        if (m_jsonBranches[i].id == id) {
            m_jsonBranches.removeAt(i);
            saveJson();
            return true;
        }
    }
    return false;
}

int ParallelWorldStore::cleanupAbandoned(int days)
{
    const qint64 cutoffMs = nowMs() - static_cast<qint64>(days) * 24 * 3600 * 1000;

    if (m_hasSqlite) {
        QSqlDatabase db = QSqlDatabase::database(QLatin1String(CONNECTION_NAME));
        QSqlQuery q(db);
        q.prepare(QStringLiteral(
            "DELETE FROM parallel_worlds WHERE status = 'abandoned' AND updatedAt < ?"));
        q.addBindValue(cutoffMs);
        q.exec();
        return q.numRowsAffected();
    }

    int removed = 0;
    QList<ParallelBranch> kept;
    for (const ParallelBranch &b : m_jsonBranches) {
        if (b.status == QStringLiteral("abandoned") && b.updatedAt < cutoffMs) {
            ++removed;
        } else {
            kept.append(b);
        }
    }
    m_jsonBranches = kept;
    if (removed > 0)
        saveJson();
    return removed;
}
