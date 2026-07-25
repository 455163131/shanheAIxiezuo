#include <QtTest>
#include "parallelworldstore.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QStandardPaths>

class TestParallelWorldStore : public QObject
{
    Q_OBJECT
private slots:
    void storageModeDetection();
    void createAndGetBranch();
    void listBranchesByChapter();
    void listBranchesWithStatusFilter();
    void updateContent();
    void renameBranch();
    void markMerged();
    void markAbandonedAndReactivate();
    void deleteBranch();
    void cleanupAbandoned();
    void parentChildRelationship();
    void jsonFallbackFifoLimit();
};

void TestParallelWorldStore::storageModeDetection()
{
    ParallelWorldStore &store = ParallelWorldStore::instance();
    QString mode = store.storageMode();
    QVERIFY(mode == "sqlite" || mode == "json");
    if (mode == "sqlite")
        QVERIFY(store.hasSqlite());
    else
        QVERIFY(!store.hasSqlite());
}

void TestParallelWorldStore::createAndGetBranch()
{
    ParallelWorldStore &store = ParallelWorldStore::instance();

    qlonglong id = store.createBranch("novel_pw_001", "ch01", 0, "主线分支",
                                       "初始内容快照", 100);
    QVERIFY(id > 0);

    ParallelBranch b = store.getBranch(id);
    QCOMPARE(b.id, id);
    QCOMPARE(b.novelId, QString("novel_pw_001"));
    QCOMPARE(b.chapterId, QString("ch01"));
    QCOMPARE(b.parentId, 0LL);
    QCOMPARE(b.branchName, QString("主线分支"));
    QCOMPARE(b.baseVersion, QString("main"));
    QCOMPARE(b.contentSnapshot, QString("初始内容快照"));
    QCOMPARE(b.currentContent, QString("初始内容快照"));
    QCOMPARE(b.status, QString("active"));
    QCOMPARE(b.wordCount, 100);
    QVERIFY(b.createdAt > 0);
    QVERIFY(b.updatedAt > 0);
    QVERIFY(b.updatedAt >= b.createdAt);

    ParallelBranch invalid = store.getBranch(-999);
    QCOMPARE(invalid.id, -1LL);
}

void TestParallelWorldStore::listBranchesByChapter()
{
    ParallelWorldStore &store = ParallelWorldStore::instance();

    qlonglong id1 = store.createBranch("novel_pw_list", "ch01", 0, "分支1", "内容1", 100);
    qlonglong id2 = store.createBranch("novel_pw_list", "ch01", 0, "分支2", "内容2", 200);
    qlonglong id3 = store.createBranch("novel_pw_list", "ch02", 0, "分支3", "内容3", 300);

    QVERIFY(id1 > 0 && id2 > 0 && id3 > 0);

    QList<ParallelBranch> ch01Branches = store.listBranches("novel_pw_list", "ch01");
    QVERIFY(ch01Branches.size() >= 2);

    bool found1 = false, found2 = false;
    for (const ParallelBranch &b : ch01Branches) {
        if (b.id == id1) found1 = true;
        if (b.id == id2) found2 = true;
    }
    QVERIFY(found1);
    QVERIFY(found2);

    QList<ParallelBranch> ch02Branches = store.listBranches("novel_pw_list", "ch02");
    bool found3 = false;
    for (const ParallelBranch &b : ch02Branches) {
        if (b.id == id3) found3 = true;
    }
    QVERIFY(found3);

    QList<ParallelBranch> empty = store.listBranches("novel_pw_list", "ch99");
    QVERIFY(empty.isEmpty());
}

void TestParallelWorldStore::listBranchesWithStatusFilter()
{
    ParallelWorldStore &store = ParallelWorldStore::instance();

    qlonglong id1 = store.createBranch("novel_pw_status", "ch01", 0, "活跃分支", "内容A", 100);
    qlonglong id2 = store.createBranch("novel_pw_status", "ch01", 0, "合并分支", "内容B", 200);
    qlonglong id3 = store.createBranch("novel_pw_status", "ch01", 0, "弃用分支", "内容C", 300);

    store.markMerged(id2);
    store.markAbandoned(id3);

    QList<ParallelBranch> activeBranches = store.listBranches("novel_pw_status", "ch01", "active");
    bool foundActive = false;
    for (const ParallelBranch &b : activeBranches) {
        if (b.id == id1) { foundActive = true; break; }
    }
    QVERIFY(foundActive);

    QList<ParallelBranch> mergedBranches = store.listBranches("novel_pw_status", "ch01", "merged");
    bool foundMerged = false;
    for (const ParallelBranch &b : mergedBranches) {
        if (b.id == id2) { foundMerged = true; break; }
    }
    QVERIFY(foundMerged);

    QList<ParallelBranch> abandonedBranches = store.listBranches("novel_pw_status", "ch01", "abandoned");
    bool foundAbandoned = false;
    for (const ParallelBranch &b : abandonedBranches) {
        if (b.id == id3) { foundAbandoned = true; break; }
    }
    QVERIFY(foundAbandoned);
}

void TestParallelWorldStore::updateContent()
{
    ParallelWorldStore &store = ParallelWorldStore::instance();

    qlonglong id = store.createBranch("novel_pw_update", "ch01", 0, "测试更新",
                                       "原始内容", 50);
    QVERIFY(id > 0);

    qint64 beforeUpdate = store.getBranch(id).updatedAt;

    bool ok = store.updateContent(id, "更新后的内容", 150);
    QVERIFY(ok);

    ParallelBranch updated = store.getBranch(id);
    QCOMPARE(updated.currentContent, QString("更新后的内容"));
    QCOMPARE(updated.wordCount, 150);
    QCOMPARE(updated.contentSnapshot, QString("原始内容"));
    QVERIFY(updated.updatedAt >= beforeUpdate);

    bool okInvalid = store.updateContent(-999, "无效", 0);
    QVERIFY(!okInvalid);
}

void TestParallelWorldStore::renameBranch()
{
    ParallelWorldStore &store = ParallelWorldStore::instance();

    qlonglong id = store.createBranch("novel_pw_rename", "ch01", 0, "旧名字",
                                       "内容", 100);
    QVERIFY(id > 0);

    bool ok = store.renameBranch(id, "新名字");
    QVERIFY(ok);

    ParallelBranch renamed = store.getBranch(id);
    QCOMPARE(renamed.branchName, QString("新名字"));

    bool okInvalid = store.renameBranch(-999, "无效");
    QVERIFY(!okInvalid);
}

void TestParallelWorldStore::markMerged()
{
    ParallelWorldStore &store = ParallelWorldStore::instance();

    qlonglong id = store.createBranch("novel_pw_merge", "ch01", 0, "待合并",
                                       "内容", 100);
    QVERIFY(id > 0);

    bool ok = store.markMerged(id);
    QVERIFY(ok);

    ParallelBranch merged = store.getBranch(id);
    QCOMPARE(merged.status, QString("merged"));

    bool okInvalid = store.markMerged(-999);
    QVERIFY(!okInvalid);
}

void TestParallelWorldStore::markAbandonedAndReactivate()
{
    ParallelWorldStore &store = ParallelWorldStore::instance();

    qlonglong id = store.createBranch("novel_pw_abandon", "ch01", 0, "待弃用",
                                       "内容", 100);
    QVERIFY(id > 0);

    bool okAbandon = store.markAbandoned(id);
    QVERIFY(okAbandon);
    QCOMPARE(store.getBranch(id).status, QString("abandoned"));

    bool okReactivate = store.reactivate(id);
    QVERIFY(okReactivate);
    QCOMPARE(store.getBranch(id).status, QString("active"));

    bool okInvalid = store.markAbandoned(-999);
    QVERIFY(!okInvalid);
    bool okInvalid2 = store.reactivate(-999);
    QVERIFY(!okInvalid2);
}

void TestParallelWorldStore::deleteBranch()
{
    ParallelWorldStore &store = ParallelWorldStore::instance();

    qlonglong id = store.createBranch("novel_pw_delete", "ch01", 0, "待删除",
                                       "内容", 100);
    QVERIFY(id > 0);
    QVERIFY(store.getBranch(id).id == id);

    bool ok = store.deleteBranch(id);
    QVERIFY(ok);
    QCOMPARE(store.getBranch(id).id, -1LL);

    bool okInvalid = store.deleteBranch(-999);
    QVERIFY(!okInvalid);
}

void TestParallelWorldStore::cleanupAbandoned()
{
    ParallelWorldStore &store = ParallelWorldStore::instance();

    qlonglong recentId = store.createBranch("novel_pw_cleanup", "ch01", 0, "近期弃用",
                                            "内容1", 100);
    store.markAbandoned(recentId);

    qlonglong oldId = store.createBranch("novel_pw_cleanup", "ch02", 0, "早期弃用",
                                         "内容2", 200);

    store.markAbandoned(oldId);

    QList<ParallelBranch> before = store.listBranches("novel_pw_cleanup", "ch02", "abandoned");
    bool foundOldBefore = false;
    for (const ParallelBranch &b : before) {
        if (b.id == oldId) { foundOldBefore = true; break; }
    }
    QVERIFY(foundOldBefore);

    int removed = store.cleanupAbandoned(30);
    QVERIFY(removed >= 0);

    QList<ParallelBranch> afterRecent = store.listBranches("novel_pw_cleanup", "ch01", "abandoned");
    bool foundRecent = false;
    for (const ParallelBranch &b : afterRecent) {
        if (b.id == recentId) { foundRecent = true; break; }
    }
    QVERIFY(foundRecent);
}

void TestParallelWorldStore::parentChildRelationship()
{
    ParallelWorldStore &store = ParallelWorldStore::instance();

    qlonglong parentId = store.createBranch("novel_pw_parent", "ch01", 0, "父分支",
                                            "父内容", 100);
    QVERIFY(parentId > 0);

    qlonglong childId = store.createBranch("novel_pw_parent", "ch01", parentId, "子分支",
                                           "子内容快照", 120);
    QVERIFY(childId > 0);

    ParallelBranch parent = store.getBranch(parentId);
    QCOMPARE(parent.parentId, 0LL);
    QCOMPARE(parent.baseVersion, QString("main"));

    ParallelBranch child = store.getBranch(childId);
    QCOMPARE(child.parentId, parentId);
    QCOMPARE(child.baseVersion, QString::number(parentId));

    QList<ParallelBranch> all = store.listBranches("novel_pw_parent", "ch01");
    QVERIFY(all.size() >= 2);
}

void TestParallelWorldStore::jsonFallbackFifoLimit()
{
    ParallelWorldStore &store = ParallelWorldStore::instance();
    if (store.storageMode() != "json")
        return;

    QList<qlonglong> ids;
    for (int i = 0; i < 510; ++i) {
        qlonglong id = store.createBranch("novel_pw_fifo", QString("ch_%1").arg(i),
                                           0, QString("分支%1").arg(i),
                                           QString("内容%1").arg(i), i * 10);
        QVERIFY(id > 0);
        ids.append(id);
    }

    QList<ParallelBranch> all = store.listBranches("novel_pw_fifo", "ch_0");
    QVERIFY(store.getBranch(ids.first()).id == -1);

    QVERIFY(store.getBranch(ids.last()).id == ids.last());
}

QTEST_GUILESS_MAIN(TestParallelWorldStore)
#include "test_parallelworldstore.moc"
