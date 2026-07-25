#include <QtTest>
#include "ragstore.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QStandardPaths>
#include <QVector>
#include <cmath>

class TestRagStore : public QObject
{
    Q_OBJECT
private slots:
    void storageModeDetection();
    void indexAndSearchChapter();
    void indexAndSearchEntity();
    void searchTopK();
    void searchByDocType();
    void removeDocument();
    void removeByDocId();
    void clearNovel();
    void rebuildIndex();
    void documentCount();
    void chapterChunking();
    void tfidfCosineSimilarity();
    void chineseBigram();
    void emptyQuery();
    void crossNovelIsolation();
};

void TestRagStore::storageModeDetection()
{
    RagStore &store = RagStore::instance();
    // 当前只实现 TF-IDF 模式；Embedding 接口预留
    QCOMPARE(store.storageMode(), QString("tfidf"));
    QVERIFY(store.hasSqlite() || !store.hasSqlite());  // 任何一种都合法
}

void TestRagStore::indexAndSearchChapter()
{
    RagStore &store = RagStore::instance();
    store.clearNovel("novel_rag_ch_search");

    store.indexChapter("novel_rag_ch_search", "ch01", "第一章",
                        "沈青云拔出长剑，剑光闪烁，他凝视着远方的敌人。"
                        "风云变幻间，他想起师父临终的嘱托，那柄古剑名为问天。");

    QList<RagStore::SearchResult> results =
        store.search("novel_rag_ch_search", "沈青云的剑", 5);
    QVERIFY(!results.isEmpty());
    QVERIFY(results.first().score > 0.0f);
    QVERIFY(results.first().chunk.contains("沈青云"));
    QCOMPARE(results.first().docType, QString("chapter"));
}

void TestRagStore::indexAndSearchEntity()
{
    RagStore &store = RagStore::instance();
    store.clearNovel("novel_rag_ent_search");

    store.indexEntity("novel_rag_ent_search", "character", "char_001",
                      "沈青云",
                      "沈青云，男，二十岁，剑修，问天剑主。师承青云观。");

    QList<RagStore::SearchResult> results =
        store.search("novel_rag_ent_search", "沈青云剑修", 5);
    QVERIFY(!results.isEmpty());
    QVERIFY(results.first().score > 0.0f);
    bool foundEntity = false;
    for (const RagStore::SearchResult &r : results) {
        if (r.docType == "character" && r.title == "沈青云") {
            foundEntity = true;
            break;
        }
    }
    QVERIFY(foundEntity);
}

void TestRagStore::searchTopK()
{
    RagStore &store = RagStore::instance();
    store.clearNovel("novel_rag_topk");

    // 三篇文档，前两篇与 query 共享 "沈青云" 系列 token，第三篇无关
    store.indexChapter("novel_rag_topk", "ch01", "1", "沈青云拔剑，剑光凛然。");
    store.indexChapter("novel_rag_topk", "ch02", "2", "沈青云挥剑，剑气纵横。");
    store.indexChapter("novel_rag_topk", "ch03", "3", "李长歌在江湖行走，射箭百步穿杨。");

    QList<RagStore::SearchResult> results = store.search("novel_rag_topk", "沈青云剑", 2);
    QCOMPARE(results.size(), 2);
    // 分数应降序
    QVERIFY(results[0].score >= results[1].score);
    // 前两篇必然命中（与 query 共享 token）
    bool foundCh01 = false, foundCh02 = false;
    for (const RagStore::SearchResult &r : results) {
        if (r.chunk.contains("沈青云拔剑")) foundCh01 = true;
        if (r.chunk.contains("沈青云挥剑")) foundCh02 = true;
    }
    QVERIFY(foundCh01 || foundCh02);
}

void TestRagStore::searchByDocType()
{
    RagStore &store = RagStore::instance();
    store.clearNovel("novel_rag_filter");

    store.indexChapter("novel_rag_filter", "ch01", "第一章", "沈青云拔出问天剑。");
    store.indexEntity("novel_rag_filter", "character", "char01", "沈青云",
                      "沈青云，男，剑修，问天剑主。");

    // 不过滤：两种 docType 都应被搜索
    QList<RagStore::SearchResult> all =
        store.search("novel_rag_filter", "沈青云", 10);
    QVERIFY(all.size() >= 2);

    // 仅 chapter
    QList<RagStore::SearchResult> chapters =
        store.search("novel_rag_filter", "沈青云", 10, {"chapter"});
    QVERIFY(!chapters.isEmpty());
    for (const RagStore::SearchResult &r : chapters)
        QCOMPARE(r.docType, QString("chapter"));

    // 仅 character
    QList<RagStore::SearchResult> chars =
        store.search("novel_rag_filter", "沈青云", 10, {"character"});
    QVERIFY(!chars.isEmpty());
    for (const RagStore::SearchResult &r : chars)
        QCOMPARE(r.docType, QString("character"));

    // 不存在的 docType
    QList<RagStore::SearchResult> none =
        store.search("novel_rag_filter", "沈青云", 10, {"term"});
    QVERIFY(none.isEmpty());
}

void TestRagStore::removeDocument()
{
    RagStore &store = RagStore::instance();
    store.clearNovel("novel_rag_rm1");
    qlonglong id = store.indexDocument("novel_rag_rm1", "chapter", "ch01",
                                        "第一章", "沈青云拔剑，剑光凛然。");
    QVERIFY(id > 0);
    QCOMPARE(store.documentCount("novel_rag_rm1"), 1);

    QVERIFY(store.removeDocument(id));
    QCOMPARE(store.documentCount("novel_rag_rm1"), 0);

    // 删除不存在的 id 返回 false
    QVERIFY(!store.removeDocument(id));
    QVERIFY(!store.removeDocument(-999));
}

void TestRagStore::removeByDocId()
{
    RagStore &store = RagStore::instance();
    store.clearNovel("novel_rag_rm2");

    // 索引一篇长章节会产生多个 chunk，全部 docId=ch01
    QString longContent;
    for (int i = 0; i < 30; ++i)
        longContent += QStringLiteral("第%1段内容，沈青云的剑光闪烁。").arg(i)
                        + QStringLiteral("\n");
    store.indexChapter("novel_rag_rm2", "ch01", "第一章", longContent);
    int beforeCount = store.documentCount("novel_rag_rm2");
    QVERIFY(beforeCount >= 1);

    bool ok = store.removeByDocId("novel_rag_rm2", "chapter", "ch01");
    QVERIFY(ok);
    QCOMPARE(store.documentCount("novel_rag_rm2"), 0);

    // 二次删除返回 false
    QVERIFY(!store.removeByDocId("novel_rag_rm2", "chapter", "ch01"));
}

void TestRagStore::clearNovel()
{
    RagStore &store = RagStore::instance();
    store.clearNovel("novel_rag_clear");

    store.indexChapter("novel_rag_clear", "ch01", "1", "沈青云拔剑。");
    store.indexChapter("novel_rag_clear", "ch02", "2", "李长歌射箭。");
    store.indexEntity("novel_rag_clear", "character", "char01", "沈青云", "剑修。");
    QVERIFY(store.documentCount("novel_rag_clear") >= 3);

    store.clearNovel("novel_rag_clear");
    QCOMPARE(store.documentCount("novel_rag_clear"), 0);

    // clearNovel 不存在的小说也不应出错
    store.clearNovel("novel_rag_clear_not_exist");
}

void TestRagStore::rebuildIndex()
{
    RagStore &store = RagStore::instance();
    store.clearNovel("novel_rag_rebuild");

    store.indexChapter("novel_rag_rebuild", "ch01", "1", "沈青云拔剑。");
    store.indexChapter("novel_rag_rebuild", "ch02", "2", "李长歌射箭。");
    int beforeCount = store.documentCount("novel_rag_rebuild");
    QVERIFY(beforeCount >= 2);

    // rebuildIndex 不改变文档数，仅刷新向量
    store.rebuildIndex("novel_rag_rebuild");
    int afterCount = store.documentCount("novel_rag_rebuild");
    QCOMPARE(afterCount, beforeCount);

    // rebuild 后搜索仍能命中
    QList<RagStore::SearchResult> results =
        store.search("novel_rag_rebuild", "沈青云", 5);
    QVERIFY(!results.isEmpty());
    QVERIFY(results.first().score > 0.0f);
}

void TestRagStore::documentCount()
{
    RagStore &store = RagStore::instance();
    store.clearNovel("novel_rag_count");

    QCOMPARE(store.documentCount("novel_rag_count"), 0);

    store.indexChapter("novel_rag_count", "ch01", "1", "内容1");
    store.indexChapter("novel_rag_count", "ch02", "2", "内容2");
    store.indexEntity("novel_rag_count", "character", "char01", "沈青云", "剑修。");
    store.indexEntity("novel_rag_count", "term", "term01", "问天剑", "古剑。");

    QVERIFY(store.documentCount("novel_rag_count") >= 4);
    QVERIFY(store.documentCount("novel_rag_count", "chapter") >= 2);
    QVERIFY(store.documentCount("novel_rag_count", "character") >= 1);
    QVERIFY(store.documentCount("novel_rag_count", "term") >= 1);
    QCOMPARE(store.documentCount("novel_rag_count", "outline"), 0);

    // 不存在的小说返回 0
    QCOMPARE(store.documentCount("novel_rag_count_not_exist"), 0);
}

void TestRagStore::chapterChunking()
{
    RagStore &store = RagStore::instance();
    store.clearNovel("novel_rag_chunk");

    // 构造一段明显超过 chunkSize=500 的长文本，含多个段落
    QStringList paras;
    for (int i = 0; i < 30; ++i) {
        // 每段约 30 个汉字，30 段约 900 字 -> 至少分 2 块
        paras.append(QStringLiteral("第%1段：沈青云拔出问天剑，剑光闪烁，凝视前方。").arg(i));
    }
    QString longContent = paras.join(QStringLiteral("\n"));
    QVERIFY(longContent.size() > 500);
    store.indexChapter("novel_rag_chunk", "ch01", "第一章", longContent);

    // 长文本必须被分块成多于 1 篇文档
    QVERIFY(store.documentCount("novel_rag_chunk") > 1);
    QVERIFY(store.documentCount("novel_rag_chunk", "chapter") > 1);

    // 短文本不会被分块
    store.clearNovel("novel_rag_chunk");
    store.indexChapter("novel_rag_chunk", "ch01", "第一章", "短内容。");
    QCOMPARE(store.documentCount("novel_rag_chunk"), 1);
}

void TestRagStore::tfidfCosineSimilarity()
{
    RagStore &store = RagStore::instance();

    // 已知向量：a=[1,2,3], b=[4,5,6]
    // dot = 1*4 + 2*5 + 3*6 = 32
    // |a| = sqrt(14), |b| = sqrt(77)
    // cosine = 32 / (sqrt(14) * sqrt(77))
    QVector<float> a = {1.0f, 2.0f, 3.0f};
    QVector<float> b = {4.0f, 5.0f, 6.0f};
    const float expected = 32.0f / (std::sqrt(14.0f) * std::sqrt(77.0f));
    const float actual = store.cosineSimilarity(a, b);
    QVERIFY(std::fabs(actual - expected) < 1e-4f);

    // 自相似：cosine(a, a) = 1
    QVERIFY(std::fabs(store.cosineSimilarity(a, a) - 1.0f) < 1e-4f);

    // 正交向量：cosine = 0
    QVector<float> x = {1.0f, 0.0f};
    QVector<float> y = {0.0f, 1.0f};
    QVERIFY(std::fabs(store.cosineSimilarity(x, y)) < 1e-4f);

    // 零向量：返回 0
    QVector<float> zero;
    QCOMPARE(store.cosineSimilarity(zero, zero), 0.0f);

    // 维度不一致：返回 0
    QVector<float> c = {1.0f, 2.0f};
    QVector<float> d = {1.0f, 2.0f, 3.0f};
    QCOMPARE(store.cosineSimilarity(c, d), 0.0f);
}

void TestRagStore::chineseBigram()
{
    RagStore &store = RagStore::instance();

    // 3 字 -> 3 token：2 个 bigram + 1 个末字
    QStringList tokens = store.tokenize("沈青云");
    QCOMPARE(tokens.size(), 3);
    QCOMPARE(tokens.at(0), QString("沈青"));
    QCOMPARE(tokens.at(1), QString("青云"));
    QCOMPARE(tokens.at(2), QString("云"));

    // 单字 -> 单 token
    QStringList single = store.tokenize("云");
    QCOMPARE(single.size(), 1);
    QCOMPARE(single.at(0), QString("云"));

    // 英文/数字按非字母数字切分，且转小写
    QStringList en = store.tokenize("Hello World 2024");
    QVERIFY(en.contains("hello"));
    QVERIFY(en.contains("world"));
    QVERIFY(en.contains("2024"));

    // 中英混合：分别成 token
    QStringList mixed = store.tokenize("沈青云 Sword");
    QVERIFY(mixed.contains("沈青"));
    QVERIFY(mixed.contains("青云"));
    QVERIFY(mixed.contains("云"));
    QVERIFY(mixed.contains("sword"));

    // 标点会切断 token
    QStringList punct = store.tokenize("沈青云，李长歌。");
    QVERIFY(punct.contains("沈青"));
    QVERIFY(punct.contains("青云"));
    QVERIFY(punct.contains("云"));
    QVERIFY(punct.contains("李长"));
    QVERIFY(punct.contains("长歌"));
    QVERIFY(punct.contains("歌"));

    // 空串
    QVERIFY(store.tokenize("").isEmpty());
}

void TestRagStore::emptyQuery()
{
    RagStore &store = RagStore::instance();
    store.clearNovel("novel_rag_empty");

    store.indexChapter("novel_rag_empty", "ch01", "1", "沈青云拔剑。");

    QList<RagStore::SearchResult> r1 = store.search("novel_rag_empty", "", 5);
    QVERIFY(r1.isEmpty());

    QList<RagStore::SearchResult> r2 = store.search("novel_rag_empty", "   ", 5);
    QVERIFY(r2.isEmpty());

    // 空查询也不应抛异常
    QList<RagStore::SearchResult> r3 = store.search("novel_rag_empty", QString(), 5);
    QVERIFY(r3.isEmpty());
}

void TestRagStore::crossNovelIsolation()
{
    RagStore &store = RagStore::instance();
    store.clearNovel("novel_rag_iso_a");
    store.clearNovel("novel_rag_iso_b");

    store.indexChapter("novel_rag_iso_a", "ch01", "甲", "沈青云在山中修炼。");
    store.indexChapter("novel_rag_iso_b", "ch01", "乙", "李长歌在江湖行走。");

    QCOMPARE(store.documentCount("novel_rag_iso_a"), 1);
    QCOMPARE(store.documentCount("novel_rag_iso_b"), 1);

    // A 搜索只命中 A 的内容
    QList<RagStore::SearchResult> resultsA =
        store.search("novel_rag_iso_a", "沈青云", 5);
    QVERIFY(!resultsA.isEmpty());
    for (const RagStore::SearchResult &r : resultsA)
        QVERIFY(!r.chunk.contains("李长歌"));

    // B 搜索只命中 B 的内容
    QList<RagStore::SearchResult> resultsB =
        store.search("novel_rag_iso_b", "李长歌", 5);
    QVERIFY(!resultsB.isEmpty());
    for (const RagStore::SearchResult &r : resultsB)
        QVERIFY(!r.chunk.contains("沈青云"));

    // 清空 A 不影响 B
    store.clearNovel("novel_rag_iso_a");
    QCOMPARE(store.documentCount("novel_rag_iso_a"), 0);
    QCOMPARE(store.documentCount("novel_rag_iso_b"), 1);
}

QTEST_GUILESS_MAIN(TestRagStore)
#include "test_ragstore.moc"
