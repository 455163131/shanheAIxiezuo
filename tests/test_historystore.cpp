#include <QtTest>
#include "historystore.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QStandardPaths>

class TestHistoryStore : public QObject
{
    Q_OBJECT
private slots:
    void insertAndQueryByChapter();
    void updateStatus();
    void queryByStatus();
    void cleanupOldJobs();
    void jsonFallbackMode();
};

void TestHistoryStore::insertAndQueryByChapter()
{
    HistoryStore &store = HistoryStore::instance();
    QVERIFY(store.storageMode() == "sqlite" || store.storageMode() == "json");

    QVariantMap job;
    job["novelId"] = "novel_001";
    job["chapterId"] = "ch03";
    job["jobKind"] = "continue";
    job["model"] = "doubao-pro";
    job["persona"] = "default";
    job["status"] = "pending";
    job["wordCount"] = 0;
    job["createdAt"] = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);

    qlonglong id = store.insertJob(job);
    QVERIFY(id > 0);

    QVariantMap saved = store.getJob(id);
    QCOMPARE(saved["novelId"].toString(), QString("novel_001"));
    QCOMPARE(saved["chapterId"].toString(), QString("ch03"));
    QCOMPARE(saved["jobKind"].toString(), QString("continue"));
    QCOMPARE(saved["status"].toString(), QString("pending"));

    QVariantList byChapter = store.queryByChapter("novel_001", "ch03", 10);
    QVERIFY(byChapter.size() >= 1);
    bool found = false;
    for (const QVariant &v : byChapter) {
        if (v.toMap()["id"].toLongLong() == id) {
            found = true;
            break;
        }
    }
    QVERIFY(found);

    QVariantList empty = store.queryByChapter("novel_001", "ch99", 10);
    QVERIFY(empty.isEmpty());
}

void TestHistoryStore::updateStatus()
{
    HistoryStore &store = HistoryStore::instance();

    QVariantMap job;
    job["novelId"] = "novel_002";
    job["chapterId"] = "ch01";
    job["jobKind"] = "continue";
    job["model"] = "doubao-pro";
    job["persona"] = "default";
    job["status"] = "pending";
    job["wordCount"] = 0;
    job["createdAt"] = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);

    qlonglong id = store.insertJob(job);
    QVERIFY(id > 0);

    store.updateStatus(id, "running");
    QVariantMap running = store.getJob(id);
    QCOMPARE(running["status"].toString(), QString("running"));

    store.updateStatus(id, "failed", "network timeout");
    QVariantMap failed = store.getJob(id);
    QCOMPARE(failed["status"].toString(), QString("failed"));
    QCOMPARE(failed["error"].toString(), QString("network timeout"));
}

void TestHistoryStore::queryByStatus()
{
    HistoryStore &store = HistoryStore::instance();

    QVariantMap j1;
    j1["novelId"] = "novel_s";
    j1["chapterId"] = "ch01";
    j1["jobKind"] = "continue";
    j1["model"] = "doubao-pro";
    j1["persona"] = "default";
    j1["status"] = "success";
    j1["wordCount"] = 1500;
    j1["createdAt"] = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
    qlonglong id1 = store.insertJob(j1);

    QVariantMap j2;
    j2["novelId"] = "novel_s";
    j2["chapterId"] = "ch02";
    j2["jobKind"] = "continue";
    j2["model"] = "doubao-pro";
    j2["persona"] = "default";
    j2["status"] = "success";
    j2["wordCount"] = 2000;
    j2["createdAt"] = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
    qlonglong id2 = store.insertJob(j2);

    QVariantMap j3;
    j3["novelId"] = "novel_s";
    j3["chapterId"] = "ch03";
    j3["jobKind"] = "continue";
    j3["model"] = "doubao-pro";
    j3["persona"] = "default";
    j3["status"] = "failed";
    j3["wordCount"] = 0;
    j3["createdAt"] = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
    qlonglong id3 = store.insertJob(j3);

    QVariantList successJobs = store.queryByStatus("success", 50);
    QVERIFY(successJobs.size() >= 2);

    int successCount = 0;
    bool hasId1 = false, hasId2 = false;
    for (const QVariant &v : successJobs) {
        QVariantMap m = v.toMap();
        if (m["status"].toString() == "success") {
            ++successCount;
            if (m["id"].toLongLong() == id1) hasId1 = true;
            if (m["id"].toLongLong() == id2) hasId2 = true;
        }
    }
    QVERIFY(successCount >= 2);
    QVERIFY(hasId1);
    QVERIFY(hasId2);

    QVariantList failedJobs = store.queryByStatus("failed", 50);
    bool hasId3 = false;
    for (const QVariant &v : failedJobs) {
        if (v.toMap()["id"].toLongLong() == id3) {
            hasId3 = true;
            break;
        }
    }
    QVERIFY(hasId3);
}

void TestHistoryStore::cleanupOldJobs()
{
    HistoryStore &store = HistoryStore::instance();

    QDateTime oldDate = QDateTime::currentDateTimeUtc().addDays(-40);

    QVariantMap oldJob;
    oldJob["novelId"] = "novel_old";
    oldJob["chapterId"] = "ch01";
    oldJob["jobKind"] = "continue";
    oldJob["model"] = "doubao-pro";
    oldJob["persona"] = "default";
    oldJob["status"] = "success";
    oldJob["wordCount"] = 1000;
    oldJob["createdAt"] = oldDate.toString(Qt::ISODate);
    qlonglong oldId = store.insertJob(oldJob);

    QVERIFY(!store.getJob(oldId).isEmpty());

    store.cleanupOldJobs(30);

    QVERIFY(store.getJob(oldId).isEmpty());
}

void TestHistoryStore::jsonFallbackMode()
{
    HistoryStore &store = HistoryStore::instance();
    QString mode = store.storageMode();
    QVERIFY(mode == "sqlite" || mode == "json");

    if (mode == "json") {
        QVERIFY(!store.hasSqlite());

        QVariantList initialJobs;
        for (int i = 0; i < 510; ++i) {
            QVariantMap job;
            job["novelId"] = QString("novel_fifo_%1").arg(i);
            job["chapterId"] = "ch01";
            job["jobKind"] = "continue";
            job["model"] = "doubao-pro";
            job["persona"] = "default";
            job["status"] = "success";
            job["wordCount"] = 1000;
            job["createdAt"] = QDateTime::currentDateTimeUtc().addDays(-i).toString(Qt::ISODate);
            qlonglong id = store.insertJob(job);
            QVERIFY(id > 0);
            initialJobs.append(QVariantMap{{"id", id}, {"novelId", job["novelId"]}});
        }

        QVariantList recent = store.queryByStatus("success", 600);
        QVERIFY(recent.size() <= 500);

        qlonglong firstId = initialJobs.first().toMap()["id"].toLongLong();
        QVERIFY(store.getJob(firstId).isEmpty());

        qlonglong lastId = initialJobs.last().toMap()["id"].toLongLong();
        QVERIFY(!store.getJob(lastId).isEmpty());
    } else {
        QVERIFY(store.hasSqlite());
        QVariantMap job;
        job["novelId"] = "novel_sqlite_check";
        job["chapterId"] = "ch01";
        job["jobKind"] = "continue";
        job["model"] = "doubao-pro";
        job["persona"] = "default";
        job["status"] = "pending";
        job["wordCount"] = 0;
        job["createdAt"] = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
        qlonglong id = store.insertJob(job);
        QVERIFY(id > 0);
        QCOMPARE(store.getJob(id)["novelId"].toString(), QString("novel_sqlite_check"));
    }
}

QTEST_GUILESS_MAIN(TestHistoryStore)
#include "test_historystore.moc"
