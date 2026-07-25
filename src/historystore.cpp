#include "historystore.h"

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
constexpr int MAX_JSON_JOBS = 500;
const char *CONNECTION_NAME = "history_store";

QString dbPath()
{
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
           + QStringLiteral("/history.db");
}

QVariantMap jobFromSqlQuery(QSqlQuery &q)
{
    QVariantMap m;
    m[QStringLiteral("id")] = q.value(0);
    m[QStringLiteral("novelId")] = q.value(1);
    m[QStringLiteral("chapterId")] = q.value(2);
    m[QStringLiteral("jobKind")] = q.value(3);
    m[QStringLiteral("model")] = q.value(4);
    m[QStringLiteral("persona")] = q.value(5);
    m[QStringLiteral("status")] = q.value(6);
    m[QStringLiteral("wordCount")] = q.value(7);
    m[QStringLiteral("output")] = q.value(8);
    m[QStringLiteral("error")] = q.value(9);
    m[QStringLiteral("finishReason")] = q.value(10);
    m[QStringLiteral("createdAt")] = q.value(11);
    m[QStringLiteral("updatedAt")] = q.value(12);
    return m;
}
} // namespace

HistoryStore &HistoryStore::instance()
{
    static HistoryStore s_instance;
    return s_instance;
}

HistoryStore::HistoryStore()
    : m_hasSqlite(false)
{
    initSqlite();
    if (!m_hasSqlite) {
        m_jsonPath = jsonPath();
        loadJson();
    }
}

bool HistoryStore::hasSqlite() const
{
    return m_hasSqlite;
}

QString HistoryStore::storageMode() const
{
    return m_hasSqlite ? QStringLiteral("sqlite") : QStringLiteral("json");
}
void HistoryStore::initSqlite()
{
    if (!QSqlDatabase::isDriverAvailable(QStringLiteral("QSQLITE")))
        return;

    QDir().mkpath(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation));

    QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"),
                                                 QLatin1String(CONNECTION_NAME));
    db.setDatabaseName(dbPath());
    if (!db.open()) {
        qWarning() << "HistoryStore: SQLite open failed:" << db.lastError().text();
        QSqlDatabase::removeDatabase(QLatin1String(CONNECTION_NAME));
        return;
    }

    QSqlQuery q(db);
    const QString createTable = QStringLiteral(
        "CREATE TABLE IF NOT EXISTS generation_jobs ("
        "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  novelId TEXT NOT NULL,"
        "  chapterId TEXT NOT NULL,"
        "  jobKind TEXT NOT NULL,"
        "  model TEXT,"
        "  persona TEXT,"
        "  status TEXT NOT NULL DEFAULT 'pending',"
        "  wordCount INTEGER DEFAULT 0,"
        "  output TEXT,"
        "  error TEXT,"
        "  finishReason TEXT,"
        "  createdAt TEXT NOT NULL,"
        "  updatedAt TEXT NOT NULL"
        ")");
    if (!q.exec(createTable)) {
        qWarning() << "HistoryStore: create table failed:" << q.lastError().text();
        db.close();
        QSqlDatabase::removeDatabase(QLatin1String(CONNECTION_NAME));
        return;
    }

    q.exec(QStringLiteral(
        "CREATE INDEX IF NOT EXISTS idx_jobs_chapter "
        "ON generation_jobs(novelId, chapterId)"));
    q.exec(QStringLiteral(
        "CREATE INDEX IF NOT EXISTS idx_jobs_status "
        "ON generation_jobs(status)"));

    m_hasSqlite = true;
}

QString HistoryStore::jsonPath()
{
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
           + QStringLiteral("/history.json");
}

void HistoryStore::loadJson()
{
    QFile f(m_jsonPath);
    if (!f.open(QIODevice::ReadOnly))
        return;
    const QJsonDocument doc = QJsonDocument::fromJson(f.readAll());
    const QVariantMap root = doc.toVariant().toMap();
    m_jsonJobs = root.value(QStringLiteral("jobs")).toList();
}

void HistoryStore::saveJson()
{
    QDir().mkpath(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation));
    QVariantMap root;
    root[QStringLiteral("schemaVersion")] = SCHEMA_VERSION;
    root[QStringLiteral("jobs")] = m_jsonJobs;
    QFile f(m_jsonPath);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate))
        return;
    const QJsonDocument doc = QJsonDocument::fromVariant(root);
    f.write(doc.toJson(QJsonDocument::Indented));
}
qlonglong HistoryStore::insertJob(const QVariantMap &jobData)
{
    const QString now = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);

    if (m_hasSqlite) {
        QSqlDatabase db = QSqlDatabase::database(QLatin1String(CONNECTION_NAME));
        QSqlQuery q(db);
        q.prepare(QStringLiteral(
            "INSERT INTO generation_jobs "
            "(novelId, chapterId, jobKind, model, persona, status, wordCount, "
            " output, error, finishReason, createdAt, updatedAt) "
            "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)"));
        q.addBindValue(jobData.value(QStringLiteral("novelId")));
        q.addBindValue(jobData.value(QStringLiteral("chapterId")));
        q.addBindValue(jobData.value(QStringLiteral("jobKind")));
        q.addBindValue(jobData.value(QStringLiteral("model")));
        q.addBindValue(jobData.value(QStringLiteral("persona")));
        q.addBindValue(jobData.value(QStringLiteral("status"), QStringLiteral("pending")));
        q.addBindValue(jobData.value(QStringLiteral("wordCount"), 0));
        q.addBindValue(jobData.value(QStringLiteral("output")));
        q.addBindValue(jobData.value(QStringLiteral("error")));
        q.addBindValue(jobData.value(QStringLiteral("finishReason")));
        q.addBindValue(jobData.value(QStringLiteral("createdAt"), now));
        q.addBindValue(now);
        if (!q.exec()) {
            qWarning() << "HistoryStore insertJob:" << q.lastError().text();
            return -1;
        }
        return q.lastInsertId().toLongLong();
    }

    qlonglong newId = 1;
    if (!m_jsonJobs.isEmpty()) {
        qlonglong maxId = 0;
        for (const QVariant &v : m_jsonJobs) {
            qlonglong id = v.toMap().value(QStringLiteral("id")).toLongLong();
            if (id > maxId)
                maxId = id;
        }
        newId = maxId + 1;
    }

    QVariantMap job = jobData;
    job[QStringLiteral("id")] = newId;
    if (!job.contains(QStringLiteral("status")))
        job[QStringLiteral("status")] = QStringLiteral("pending");
    if (!job.contains(QStringLiteral("wordCount")))
        job[QStringLiteral("wordCount")] = 0;
    if (!job.contains(QStringLiteral("createdAt")))
        job[QStringLiteral("createdAt")] = now;
    job[QStringLiteral("updatedAt")] = now;

    m_jsonJobs.append(job);

    while (m_jsonJobs.size() > MAX_JSON_JOBS)
        m_jsonJobs.removeFirst();

    saveJson();
    return newId;
}

QVariantList HistoryStore::queryByChapter(const QString &novelId, const QString &chapterId, int limit)
{
    QVariantList result;

    if (m_hasSqlite) {
        QSqlDatabase db = QSqlDatabase::database(QLatin1String(CONNECTION_NAME));
        QSqlQuery q(db);
        q.prepare(QStringLiteral(
            "SELECT id, novelId, chapterId, jobKind, model, persona, status, "
            "       wordCount, output, error, finishReason, createdAt, updatedAt "
            "FROM generation_jobs "
            "WHERE novelId = ? AND chapterId = ? "
            "ORDER BY id DESC LIMIT ?"));
        q.addBindValue(novelId);
        q.addBindValue(chapterId);
        q.addBindValue(limit);
        if (q.exec()) {
            while (q.next())
                result.append(jobFromSqlQuery(q));
        }
        return result;
    }

    int count = 0;
    for (int i = m_jsonJobs.size() - 1; i >= 0 && count < limit; --i) {
        QVariantMap m = m_jsonJobs.at(i).toMap();
        if (m.value(QStringLiteral("novelId")).toString() == novelId
            && m.value(QStringLiteral("chapterId")).toString() == chapterId) {
            result.append(m);
            ++count;
        }
    }
    return result;
}

QVariantList HistoryStore::queryByStatus(const QString &status, int limit)
{
    QVariantList result;

    if (m_hasSqlite) {
        QSqlDatabase db = QSqlDatabase::database(QLatin1String(CONNECTION_NAME));
        QSqlQuery q(db);
        q.prepare(QStringLiteral(
            "SELECT id, novelId, chapterId, jobKind, model, persona, status, "
            "       wordCount, output, error, finishReason, createdAt, updatedAt "
            "FROM generation_jobs "
            "WHERE status = ? "
            "ORDER BY id DESC LIMIT ?"));
        q.addBindValue(status);
        q.addBindValue(limit);
        if (q.exec()) {
            while (q.next())
                result.append(jobFromSqlQuery(q));
        }
        return result;
    }

    int count = 0;
    for (int i = m_jsonJobs.size() - 1; i >= 0 && count < limit; --i) {
        QVariantMap m = m_jsonJobs.at(i).toMap();
        if (m.value(QStringLiteral("status")).toString() == status) {
            result.append(m);
            ++count;
        }
    }
    return result;
}
QVariantMap HistoryStore::getJob(qlonglong id)
{
    if (m_hasSqlite) {
        QSqlDatabase db = QSqlDatabase::database(QLatin1String(CONNECTION_NAME));
        QSqlQuery q(db);
        q.prepare(QStringLiteral(
            "SELECT id, novelId, chapterId, jobKind, model, persona, status, "
            "       wordCount, output, error, finishReason, createdAt, updatedAt "
            "FROM generation_jobs WHERE id = ?"));
        q.addBindValue(id);
        if (q.exec() && q.next())
            return jobFromSqlQuery(q);
        return QVariantMap();
    }

    for (const QVariant &v : m_jsonJobs) {
        QVariantMap m = v.toMap();
        if (m.value(QStringLiteral("id")).toLongLong() == id)
            return m;
    }
    return QVariantMap();
}

void HistoryStore::updateStatus(qlonglong id, const QString &status, const QString &error)
{
    const QString now = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);

    if (m_hasSqlite) {
        QSqlDatabase db = QSqlDatabase::database(QLatin1String(CONNECTION_NAME));
        QSqlQuery q(db);
        if (!error.isEmpty()) {
            q.prepare(QStringLiteral(
                "UPDATE generation_jobs SET status = ?, error = ?, updatedAt = ? WHERE id = ?"));
            q.addBindValue(status);
            q.addBindValue(error);
            q.addBindValue(now);
            q.addBindValue(id);
        } else {
            q.prepare(QStringLiteral(
                "UPDATE generation_jobs SET status = ?, updatedAt = ? WHERE id = ?"));
            q.addBindValue(status);
            q.addBindValue(now);
            q.addBindValue(id);
        }
        q.exec();
        return;
    }

    for (int i = 0; i < m_jsonJobs.size(); ++i) {
        QVariantMap m = m_jsonJobs.at(i).toMap();
        if (m.value(QStringLiteral("id")).toLongLong() == id) {
            m[QStringLiteral("status")] = status;
            if (!error.isEmpty())
                m[QStringLiteral("error")] = error;
            m[QStringLiteral("updatedAt")] = now;
            m_jsonJobs[i] = m;
            saveJson();
            return;
        }
    }
}

void HistoryStore::setOutput(qlonglong id, const QString &output, int wordCount, const QString &finishReason)
{
    const QString now = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);

    if (m_hasSqlite) {
        QSqlDatabase db = QSqlDatabase::database(QLatin1String(CONNECTION_NAME));
        QSqlQuery q(db);
        q.prepare(QStringLiteral(
            "UPDATE generation_jobs SET output = ?, wordCount = ?, finishReason = ?, "
            "  updatedAt = ? WHERE id = ?"));
        q.addBindValue(output);
        q.addBindValue(wordCount);
        q.addBindValue(finishReason);
        q.addBindValue(now);
        q.addBindValue(id);
        q.exec();
        return;
    }

    for (int i = 0; i < m_jsonJobs.size(); ++i) {
        QVariantMap m = m_jsonJobs.at(i).toMap();
        if (m.value(QStringLiteral("id")).toLongLong() == id) {
            m[QStringLiteral("output")] = output;
            m[QStringLiteral("wordCount")] = wordCount;
            m[QStringLiteral("finishReason")] = finishReason;
            m[QStringLiteral("updatedAt")] = now;
            m_jsonJobs[i] = m;
            saveJson();
            return;
        }
    }
}

void HistoryStore::cleanupOldJobs(int days)
{
    const QDateTime cutoff = QDateTime::currentDateTimeUtc().addDays(-days);
    const QString cutoffStr = cutoff.toString(Qt::ISODate);

    if (m_hasSqlite) {
        QSqlDatabase db = QSqlDatabase::database(QLatin1String(CONNECTION_NAME));
        QSqlQuery q(db);
        q.prepare(QStringLiteral(
            "DELETE FROM generation_jobs WHERE createdAt < ?"));
        q.addBindValue(cutoffStr);
        q.exec();
        return;
    }

    QVariantList kept;
    for (const QVariant &v : m_jsonJobs) {
        QVariantMap m = v.toMap();
        QDateTime createdAt = QDateTime::fromString(
            m.value(QStringLiteral("createdAt")).toString(), Qt::ISODate);
        if (createdAt.isValid() && createdAt >= cutoff)
            kept.append(m);
    }
    m_jsonJobs = kept;
    saveJson();
}