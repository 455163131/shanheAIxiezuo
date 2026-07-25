#pragma once

#include <QString>
#include <QList>

struct ParallelBranch {
    qlonglong id;
    QString novelId;
    QString chapterId;
    qlonglong parentId;
    QString branchName;
    QString baseVersion;
    QString contentSnapshot;
    QString currentContent;
    QString status;
    qint64 createdAt;
    qint64 updatedAt;
    int wordCount;
};

class ParallelWorldStore {
public:
    static ParallelWorldStore &instance();

    bool hasSqlite() const;
    QString storageMode() const;

    qlonglong createBranch(const QString &novelId, const QString &chapterId,
                           qlonglong parentId, const QString &branchName,
                           const QString &contentSnapshot, int wordCount);

    QList<ParallelBranch> listBranches(const QString &novelId, const QString &chapterId,
                                       const QString &statusFilter = QString());

    ParallelBranch getBranch(qlonglong id);

    bool updateContent(qlonglong id, const QString &content, int wordCount);

    bool renameBranch(qlonglong id, const QString &newName);

    bool markMerged(qlonglong id);

    bool markAbandoned(qlonglong id);

    bool reactivate(qlonglong id);

    bool deleteBranch(qlonglong id);

    int cleanupAbandoned(int days = 30);

private:
    ParallelWorldStore();
    bool m_hasSqlite;
    QString m_jsonPath;
    QList<ParallelBranch> m_jsonBranches;

    void initSqlite();
    void loadJson();
    void saveJson();
    static QString jsonPath();
};
