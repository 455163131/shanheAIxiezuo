#include "ragstore.h"

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
#include <QDataStream>
#include <QRegularExpression>
#include <cmath>

namespace {
constexpr int SCHEMA_VERSION = 1;
constexpr int MAX_JSON_DOCS = 2000;
const char *CONNECTION_NAME = "rag_store";

bool isChineseChar(ushort u)
{
    return u >= 0x4E00 && u <= 0x9FFF;
}

QString dbPath()
{
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
           + QStringLiteral("/ragstore.db");
}

qint64 nowMs()
{
    return QDateTime::currentMSecsSinceEpoch();
}

QByteArray serializeVector(const QVector<float> &vec)
{
    QByteArray bytes;
    QDataStream stream(&bytes, QIODevice::WriteOnly);
    stream.setFloatingPointPrecision(QDataStream::SinglePrecision);
    stream.setVersion(QDataStream::Qt_6_0);
    stream << vec;
    return bytes;
}

QVector<float> deserializeVector(const QByteArray &bytes)
{
    QVector<float> vec;
    if (bytes.isEmpty())
        return vec;
    QDataStream stream(bytes);
    stream.setFloatingPointPrecision(QDataStream::SinglePrecision);
    stream.setVersion(QDataStream::Qt_6_0);
    stream >> vec;
    return vec;
}

QVariantList vectorToVariantList(const QVector<float> &vec)
{
    QVariantList list;
    list.reserve(vec.size());
    for (float v : vec)
        list.append(v);
    return list;
}

QVector<float> variantListToVector(const QVariantList &list)
{
    QVector<float> vec;
    vec.reserve(list.size());
    for (const QVariant &v : list)
        vec.append(v.toFloat());
    return vec;
}
} // namespace

RagStore &RagStore::instance()
{
    static RagStore s_instance;
    return s_instance;
}

RagStore::RagStore()
    : m_hasSqlite(false)
{
    initSqlite();
    if (!m_hasSqlite) {
        m_jsonPath = jsonPath();
        loadJson();
    }
}

bool RagStore::hasSqlite() const
{
    return m_hasSqlite;
}

QString RagStore::storageMode() const
{
    return QStringLiteral("tfidf");
}

void RagStore::initSqlite()
{
    if (!QSqlDatabase::isDriverAvailable(QStringLiteral("QSQLITE")))
        return;

    QDir().mkpath(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation));

    QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"),
                                                 QLatin1String(CONNECTION_NAME));
    db.setDatabaseName(dbPath());
    if (!db.open()) {
        qWarning() << "RagStore: SQLite open failed:" << db.lastError().text();
        QSqlDatabase::removeDatabase(QLatin1String(CONNECTION_NAME));
        return;
    }

    QSqlQuery q(db);
    const QString createTable = QStringLiteral(
        "CREATE TABLE IF NOT EXISTS rag_documents ("
        "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  novelId TEXT NOT NULL,"
        "  docType TEXT NOT NULL,"
        "  docId TEXT NOT NULL,"
        "  title TEXT,"
        "  content TEXT NOT NULL,"
        "  chunk TEXT NOT NULL,"
        "  chunkIndex INTEGER DEFAULT 0,"
        "  vector BLOB,"
        "  createdAt INTEGER NOT NULL"
        ")");
    if (!q.exec(createTable)) {
        qWarning() << "RagStore: create table failed:" << q.lastError().text();
        db.close();
        QSqlDatabase::removeDatabase(QLatin1String(CONNECTION_NAME));
        return;
    }

    q.exec(QStringLiteral(
        "CREATE INDEX IF NOT EXISTS idx_rag_novel "
        "ON rag_documents(novelId)"));
    q.exec(QStringLiteral(
        "CREATE INDEX IF NOT EXISTS idx_rag_novel_type "
        "ON rag_documents(novelId, docType)"));
    q.exec(QStringLiteral(
        "CREATE INDEX IF NOT EXISTS idx_rag_doc_id "
        "ON rag_documents(docType, docId)"));

    m_hasSqlite = true;
}

QString RagStore::jsonPath()
{
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
           + QStringLiteral("/ragstore.json");
}

void RagStore::loadJson()
{
    QFile f(m_jsonPath);
    if (!f.open(QIODevice::ReadOnly))
        return;
    const QJsonDocument doc = QJsonDocument::fromJson(f.readAll());
    const QVariantMap root = doc.toVariant().toMap();
    m_jsonDocs = root.value(QStringLiteral("documents")).toList();
}

void RagStore::saveJson()
{
    QDir().mkpath(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation));
    QVariantMap root;
    root[QStringLiteral("schemaVersion")] = SCHEMA_VERSION;
    root[QStringLiteral("documents")] = m_jsonDocs;
    QFile f(m_jsonPath);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate))
        return;
    const QJsonDocument doc = QJsonDocument::fromVariant(root);
    f.write(doc.toJson(QJsonDocument::Indented));
}

QStringList RagStore::tokenize(const QString &text) const
{
    QStringList tokens;
    QString chineseRun;
    QString asciiRun;

    auto flushChinese = [&]() {
        if (chineseRun.isEmpty())
            return;
        const int n = chineseRun.size();
        for (int i = 0; i + 1 < n; ++i)
            tokens.append(chineseRun.mid(i, 2));
        if (n >= 1)
            tokens.append(QString(chineseRun.at(n - 1)));
        chineseRun.clear();
    };

    auto flushAscii = [&]() {
        if (asciiRun.isEmpty())
            return;
        tokens.append(asciiRun.toLower());
        asciiRun.clear();
    };

    for (const QChar &ch : text) {
        const ushort u = ch.unicode();
        if (isChineseChar(u)) {
            flushAscii();
            chineseRun.append(ch);
        } else if (ch.isLetterOrNumber()) {
            flushChinese();
            asciiRun.append(ch);
        } else {
            flushChinese();
            flushAscii();
        }
    }
    flushChinese();
    flushAscii();
    return tokens;
}

QStringList RagStore::chunkText(const QString &content, int chunkSize) const
{
    if (chunkSize <= 0 || content.isEmpty())
        return QStringList{content};

    const QStringList paragraphs = content.split(QStringLiteral("\n"));
    QStringList chunks;
    QString current;

    for (const QString &rawPara : paragraphs) {
        const QString para = rawPara.trimmed();
        if (para.isEmpty())
            continue;

        if (!current.isEmpty()
            && current.size() + para.size() + 1 > chunkSize) {
            chunks.append(current.trimmed());
            current.clear();
        }

        if (current.isEmpty()) {
            current = para;
        } else {
            current += QStringLiteral("\n") + para;
        }
    }
    if (!current.trimmed().isEmpty())
        chunks.append(current.trimmed());

    return chunks.isEmpty() ? QStringList{content} : chunks;
}

QHash<QString, int> RagStore::buildVocabulary(const QString &novelId)
{
    QHash<QString, int> vocab;
    int idx = 0;

    auto addText = [&](const QString &text) {
        const QStringList tokens = tokenize(text);
        for (const QString &t : tokens) {
            if (!vocab.contains(t))
                vocab.insert(t, idx++);
        }
    };

    if (m_hasSqlite) {
        QSqlDatabase db = QSqlDatabase::database(QLatin1String(CONNECTION_NAME));
        QSqlQuery q(db);
        q.prepare(QStringLiteral("SELECT chunk FROM rag_documents WHERE novelId = ?"));
        q.addBindValue(novelId);
        if (q.exec()) {
            while (q.next())
                addText(q.value(0).toString());
        }
    } else {
        for (const QVariant &v : m_jsonDocs) {
            const QVariantMap m = v.toMap();
            if (m.value(QStringLiteral("novelId")).toString() == novelId)
                addText(m.value(QStringLiteral("chunk")).toString());
        }
    }
    return vocab;
}

double RagStore::computeIdf(const QString &term, const QString &novelId)
{
    int totalDocs = 0;
    int docsWithTerm = 0;

    if (m_hasSqlite) {
        QSqlDatabase db = QSqlDatabase::database(QLatin1String(CONNECTION_NAME));
        QSqlQuery qCount(db);
        qCount.prepare(QStringLiteral("SELECT COUNT(*) FROM rag_documents WHERE novelId = ?"));
        qCount.addBindValue(novelId);
        if (qCount.exec() && qCount.next())
            totalDocs = qCount.value(0).toInt();

        QSqlQuery qChunks(db);
        qChunks.prepare(QStringLiteral("SELECT chunk FROM rag_documents WHERE novelId = ?"));
        qChunks.addBindValue(novelId);
        if (qChunks.exec()) {
            while (qChunks.next()) {
                const QStringList tokens = tokenize(qChunks.value(0).toString());
                if (tokens.contains(term))
                    ++docsWithTerm;
            }
        }
    } else {
        for (const QVariant &v : m_jsonDocs) {
            const QVariantMap m = v.toMap();
            if (m.value(QStringLiteral("novelId")).toString() != novelId)
                continue;
            ++totalDocs;
            const QStringList tokens = tokenize(m.value(QStringLiteral("chunk")).toString());
            if (tokens.contains(term))
                ++docsWithTerm;
        }
    }

    if (totalDocs <= 0 || docsWithTerm <= 0)
        return 0.0;
    // log(N/df) + 1: 避免 df=N/2 时 idf=0 导致查询向量全零（+1 保证非零偏置）
    return std::log(static_cast<double>(totalDocs) / static_cast<double>(docsWithTerm)) + 1.0;
}

QVector<float> RagStore::computeTfidfVector(const QString &novelId, const QString &text)
{
    const QHash<QString, int> vocab = buildVocabulary(novelId);
    QVector<float> vec;
    vec.fill(0.0f, vocab.size());
    if (vocab.isEmpty())
        return vec;

    const QStringList tokens = tokenize(text);
    if (tokens.isEmpty())
        return vec;

    const int totalTerms = tokens.size();
    QHash<QString, int> termCounts;
    for (const QString &t : tokens)
        ++termCounts[t];

    for (auto it = termCounts.constBegin(); it != termCounts.constEnd(); ++it) {
        const QString &term = it.key();
        const int count = it.value();
        const auto vocabIt = vocab.constFind(term);
        if (vocabIt == vocab.constEnd())
            continue;

        const double tf = static_cast<double>(count) / static_cast<double>(totalTerms);
        const double idf = computeIdf(term, novelId);
        const double tfidf = tf * idf;
        vec[vocabIt.value()] = static_cast<float>(tfidf);
    }
    return vec;
}

float RagStore::cosineSimilarity(const QVector<float> &a, const QVector<float> &b) const
{
    if (a.size() != b.size() || a.isEmpty())
        return 0.0f;

    float dot = 0.0f;
    float normA = 0.0f;
    float normB = 0.0f;
    for (int i = 0; i < a.size(); ++i) {
        dot += a[i] * b[i];
        normA += a[i] * a[i];
        normB += b[i] * b[i];
    }
    if (normA <= 0.0f || normB <= 0.0f)
        return 0.0f;
    return dot / (std::sqrt(normA) * std::sqrt(normB));
}

qlonglong RagStore::indexDocument(const QString &novelId, const QString &docType,
                                    const QString &docId, const QString &title,
                                    const QString &content, int chunkSize)
{
    const QStringList chunks = chunkText(content, chunkSize);
    qlonglong firstId = -1;
    const qint64 now = nowMs();

    for (int i = 0; i < chunks.size(); ++i) {
        const QString &chunk = chunks.at(i);
        const QVector<float> vec = computeTfidfVector(novelId, chunk);
        qlonglong newId = -1;

        if (m_hasSqlite) {
            QSqlDatabase db = QSqlDatabase::database(QLatin1String(CONNECTION_NAME));
            QSqlQuery q(db);
            q.prepare(QStringLiteral(
                "INSERT INTO rag_documents "
                "(novelId, docType, docId, title, content, chunk, chunkIndex, vector, createdAt) "
                "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)"));
            q.addBindValue(novelId);
            q.addBindValue(docType);
            q.addBindValue(docId);
            q.addBindValue(title);
            q.addBindValue(content);
            q.addBindValue(chunk);
            q.addBindValue(i);
            q.addBindValue(serializeVector(vec));
            q.addBindValue(now);
            if (q.exec()) {
                newId = q.lastInsertId().toLongLong();
            } else {
                qWarning() << "RagStore indexDocument:" << q.lastError().text();
            }
        } else {
            newId = 1;
            if (!m_jsonDocs.isEmpty()) {
                qlonglong maxId = 0;
                for (const QVariant &v : m_jsonDocs) {
                    const qlonglong id = v.toMap().value(QStringLiteral("id")).toLongLong();
                    if (id > maxId)
                        maxId = id;
                }
                newId = maxId + 1;
            }

            QVariantMap m;
            m[QStringLiteral("id")] = newId;
            m[QStringLiteral("novelId")] = novelId;
            m[QStringLiteral("docType")] = docType;
            m[QStringLiteral("docId")] = docId;
            m[QStringLiteral("title")] = title;
            m[QStringLiteral("content")] = content;
            m[QStringLiteral("chunk")] = chunk;
            m[QStringLiteral("chunkIndex")] = i;
            m[QStringLiteral("vector")] = vectorToVariantList(vec);
            m[QStringLiteral("createdAt")] = now;

            m_jsonDocs.append(m);
            saveJson();
        }

        if (i == 0)
            firstId = newId;
    }
    return firstId;
}

void RagStore::indexChapter(const QString &novelId, const QString &chapterId,
                             const QString &title, const QString &content)
{
    indexDocument(novelId, QStringLiteral("chapter"), chapterId, title, content, 500);
}

void RagStore::indexEntity(const QString &novelId, const QString &docType,
                            const QString &docId, const QString &title,
                            const QString &content)
{
    indexDocument(novelId, docType, docId, title, content, 0);
}

bool RagStore::removeDocument(qlonglong id)
{
    if (m_hasSqlite) {
        QSqlDatabase db = QSqlDatabase::database(QLatin1String(CONNECTION_NAME));
        QSqlQuery q(db);
        q.prepare(QStringLiteral("DELETE FROM rag_documents WHERE id = ?"));
        q.addBindValue(id);
        return q.exec() && q.numRowsAffected() > 0;
    }

    for (int i = 0; i < m_jsonDocs.size(); ++i) {
        if (m_jsonDocs.at(i).toMap().value(QStringLiteral("id")).toLongLong() == id) {
            m_jsonDocs.removeAt(i);
            saveJson();
            return true;
        }
    }
    return false;
}

bool RagStore::removeByDocId(const QString &novelId, const QString &docType, const QString &docId)
{
    if (m_hasSqlite) {
        QSqlDatabase db = QSqlDatabase::database(QLatin1String(CONNECTION_NAME));
        QSqlQuery q(db);
        q.prepare(QStringLiteral(
            "DELETE FROM rag_documents WHERE novelId = ? AND docType = ? AND docId = ?"));
        q.addBindValue(novelId);
        q.addBindValue(docType);
        q.addBindValue(docId);
        if (!q.exec())
            return false;
        return q.numRowsAffected() > 0;
    }

    bool removed = false;
    QList<QVariant> kept;
    kept.reserve(m_jsonDocs.size());
    for (const QVariant &v : m_jsonDocs) {
        const QVariantMap m = v.toMap();
        if (m.value(QStringLiteral("novelId")).toString() == novelId
            && m.value(QStringLiteral("docType")).toString() == docType
            && m.value(QStringLiteral("docId")).toString() == docId) {
            removed = true;
        } else {
            kept.append(v);
        }
    }
    if (removed) {
        m_jsonDocs = kept;
        saveJson();
    }
    return removed;
}

void RagStore::clearNovel(const QString &novelId)
{
    if (m_hasSqlite) {
        QSqlDatabase db = QSqlDatabase::database(QLatin1String(CONNECTION_NAME));
        QSqlQuery q(db);
        q.prepare(QStringLiteral("DELETE FROM rag_documents WHERE novelId = ?"));
        q.addBindValue(novelId);
        q.exec();
        return;
    }

    QList<QVariant> kept;
    kept.reserve(m_jsonDocs.size());
    for (const QVariant &v : m_jsonDocs) {
        if (v.toMap().value(QStringLiteral("novelId")).toString() != novelId)
            kept.append(v);
    }
    if (kept.size() != m_jsonDocs.size()) {
        m_jsonDocs = kept;
        saveJson();
    }
}

QList<RagStore::SearchResult> RagStore::search(const QString &novelId, const QString &query,
                                                  int topK, const QStringList &docTypeFilter)
{
    QList<SearchResult> results;
    if (query.trimmed().isEmpty())
        return results;

    struct Candidate {
        qlonglong id;
        QString docType;
        QString title;
        QString chunk;
    };
    QList<Candidate> candidates;

    if (m_hasSqlite) {
        QSqlDatabase db = QSqlDatabase::database(QLatin1String(CONNECTION_NAME));
        QSqlQuery q(db);
        QString sql = QStringLiteral(
            "SELECT id, docType, title, chunk FROM rag_documents WHERE novelId = ?");
        if (!docTypeFilter.isEmpty()) {
            QStringList placeholders;
            placeholders.reserve(docTypeFilter.size());
            for (int i = 0; i < docTypeFilter.size(); ++i)
                placeholders.append(QStringLiteral("?"));
            sql += QStringLiteral(" AND docType IN (")
                   + placeholders.join(QStringLiteral(",")) + QStringLiteral(")");
        }
        q.prepare(sql);
        q.addBindValue(novelId);
        for (const QString &dt : docTypeFilter)
            q.addBindValue(dt);
        if (q.exec()) {
            while (q.next()) {
                Candidate c;
                c.id = q.value(0).toLongLong();
                c.docType = q.value(1).toString();
                c.title = q.value(2).toString();
                c.chunk = q.value(3).toString();
                candidates.append(c);
            }
        }
    } else {
        for (const QVariant &v : m_jsonDocs) {
            const QVariantMap m = v.toMap();
            if (m.value(QStringLiteral("novelId")).toString() != novelId)
                continue;
            const QString dt = m.value(QStringLiteral("docType")).toString();
            if (!docTypeFilter.isEmpty() && !docTypeFilter.contains(dt))
                continue;
            Candidate c;
            c.id = m.value(QStringLiteral("id")).toLongLong();
            c.docType = dt;
            c.title = m.value(QStringLiteral("title")).toString();
            c.chunk = m.value(QStringLiteral("chunk")).toString();
            candidates.append(c);
        }
    }

    if (candidates.isEmpty())
        return results;

    const QVector<float> queryVec = computeTfidfVector(novelId, query);

    for (const Candidate &c : candidates) {
        const QVector<float> docVec = computeTfidfVector(novelId, c.chunk);
        const float score = cosineSimilarity(queryVec, docVec);
        SearchResult r;
        r.docId = c.id;
        r.docType = c.docType;
        r.title = c.title;
        r.chunk = c.chunk;
        r.score = score;
        results.append(r);
    }

    std::sort(results.begin(), results.end(),
              [](const SearchResult &a, const SearchResult &b) {
                  return a.score > b.score;
              });
    if (results.size() > topK)
        results = results.mid(0, topK);

    return results;
}

void RagStore::rebuildIndex(const QString &novelId)
{
    if (m_hasSqlite) {
        QSqlDatabase db = QSqlDatabase::database(QLatin1String(CONNECTION_NAME));
        QSqlQuery qSelect(db);
        qSelect.prepare(QStringLiteral("SELECT id, chunk FROM rag_documents WHERE novelId = ?"));
        qSelect.addBindValue(novelId);
        if (!qSelect.exec())
            return;

        QSqlQuery qUpdate(db);
        qUpdate.prepare(QStringLiteral("UPDATE rag_documents SET vector = ? WHERE id = ?"));
        while (qSelect.next()) {
            const qlonglong id = qSelect.value(0).toLongLong();
            const QString chunk = qSelect.value(1).toString();
            const QVector<float> vec = computeTfidfVector(novelId, chunk);
            qUpdate.addBindValue(serializeVector(vec));
            qUpdate.addBindValue(id);
            qUpdate.exec();
        }
        return;
    }

    bool dirty = false;
    for (int i = 0; i < m_jsonDocs.size(); ++i) {
        QVariantMap m = m_jsonDocs.at(i).toMap();
        if (m.value(QStringLiteral("novelId")).toString() != novelId)
            continue;
        const QString chunk = m.value(QStringLiteral("chunk")).toString();
        const QVector<float> vec = computeTfidfVector(novelId, chunk);
        m[QStringLiteral("vector")] = vectorToVariantList(vec);
        m_jsonDocs[i] = m;
        dirty = true;
    }
    if (dirty)
        saveJson();
}

int RagStore::documentCount(const QString &novelId)
{
    if (m_hasSqlite) {
        QSqlDatabase db = QSqlDatabase::database(QLatin1String(CONNECTION_NAME));
        QSqlQuery q(db);
        q.prepare(QStringLiteral("SELECT COUNT(*) FROM rag_documents WHERE novelId = ?"));
        q.addBindValue(novelId);
        if (q.exec() && q.next())
            return q.value(0).toInt();
        return 0;
    }

    int count = 0;
    for (const QVariant &v : m_jsonDocs) {
        if (v.toMap().value(QStringLiteral("novelId")).toString() == novelId)
            ++count;
    }
    return count;
}

int RagStore::documentCount(const QString &novelId, const QString &docType)
{
    if (m_hasSqlite) {
        QSqlDatabase db = QSqlDatabase::database(QLatin1String(CONNECTION_NAME));
        QSqlQuery q(db);
        q.prepare(QStringLiteral(
            "SELECT COUNT(*) FROM rag_documents WHERE novelId = ? AND docType = ?"));
        q.addBindValue(novelId);
        q.addBindValue(docType);
        if (q.exec() && q.next())
            return q.value(0).toInt();
        return 0;
    }

    int count = 0;
    for (const QVariant &v : m_jsonDocs) {
        const QVariantMap m = v.toMap();
        if (m.value(QStringLiteral("novelId")).toString() == novelId
            && m.value(QStringLiteral("docType")).toString() == docType)
            ++count;
    }
    return count;
}
