#pragma once

#include <QString>
#include <QVariantList>
#include <QVariantMap>

class HistoryStore {
public:
    static HistoryStore &instance();

    bool hasSqlite() const;
    QString storageMode() const;

    qlonglong insertJob(const QVariantMap &jobData);
    QVariantList queryByChapter(const QString &novelId, const QString &chapterId, int limit = 20);
    QVariantList queryByStatus(const QString &status, int limit = 50);
    QVariantMap getJob(qlonglong id);
    void updateStatus(qlonglong id, const QString &status, const QString &error = QString());
    void setOutput(qlonglong id, const QString &output, int wordCount, const QString &finishReason);
    void cleanupOldJobs(int days = 30);

private:
    HistoryStore();
    bool m_hasSqlite;
    QString m_jsonPath;
    QVariantList m_jsonJobs;

    void initSqlite();
    void loadJson();
    void saveJson();
    static QString jsonPath();
};