#include <QtTest>
#include <QTemporaryDir>
#include "entitystore.h"

class TestEntityStore : public QObject
{
    Q_OBJECT
private slots:
    void characters_crud();
    void terms_crud();
    void knowledge_crud();
    void memos_crud();
    void outlines_crud();
    void templates_crud();
    void wordCountAutoCalc();
    void emptyDirCreatesFiles();
    void persistenceRoundtrip();
    void seedTemplates();
};

void TestEntityStore::characters_crud()
{
    QTemporaryDir tempDir;
    EntityStore store(tempDir.path());

    QVERIFY(store.characters().isEmpty());

    QVariantMap ch1;
    ch1[QStringLiteral("name")] = QStringLiteral("张三");
    ch1[QStringLiteral("content")] = QStringLiteral("男主角，性格开朗");
    ch1[QStringLiteral("category")] = QStringLiteral("主角");
    ch1[QStringLiteral("pin")] = true;
    ch1[QStringLiteral("hidden")] = false;
    int id1 = store.addCharacter(ch1);
    QVERIFY(id1 > 0);
    QCOMPARE(store.characters().size(), 1);
    QCOMPARE(store.characters().first().toMap().value(QStringLiteral("name")).toString(),
             QStringLiteral("张三"));
    QCOMPARE(store.characters().first().toMap().value(QStringLiteral("id")).toInt(), id1);

    QVariantMap ch2;
    ch2[QStringLiteral("name")] = QStringLiteral("李四");
    ch2[QStringLiteral("content")] = QStringLiteral("女主角");
    ch2[QStringLiteral("category")] = QStringLiteral("主角");
    int id2 = store.addCharacter(ch2);
    QVERIFY(id2 > id1);
    QCOMPARE(store.characters().size(), 2);

    QVariantMap update;
    update[QStringLiteral("name")] = QStringLiteral("张三丰");
    update[QStringLiteral("content")] = QStringLiteral("男主角，性格开朗，武功高强");
    store.updateCharacter(id1, update);
    QCOMPARE(store.characters().first().toMap().value(QStringLiteral("name")).toString(),
             QStringLiteral("张三丰"));

    QVariantList folders = store.characterFolders();
    QVERIFY(folders.isEmpty());
    QVariantList newFolders;
    QVariantMap folder;
    folder[QStringLiteral("name")] = QStringLiteral("主要角色");
    newFolders.append(folder);
    store.setCharacterFolders(newFolders);
    QCOMPARE(store.characterFolders().size(), 1);
    QCOMPARE(store.characterFolders().first().toMap().value(QStringLiteral("name")).toString(),
             QStringLiteral("主要角色"));

    store.deleteCharacter(id2);
    QCOMPARE(store.characters().size(), 1);
}

void TestEntityStore::terms_crud()
{
    QTemporaryDir tempDir;
    EntityStore store(tempDir.path());

    QVERIFY(store.terms().isEmpty());

    QVariantMap t1;
    t1[QStringLiteral("name")] = QStringLiteral("灵气");
    t1[QStringLiteral("content")] = QStringLiteral("修炼所需的能量");
    t1[QStringLiteral("category")] = QStringLiteral("修炼体系");
    int id1 = store.addTerm(t1);
    QVERIFY(id1 > 0);
    QCOMPARE(store.terms().size(), 1);
    QCOMPARE(store.terms().first().toMap().value(QStringLiteral("name")).toString(),
             QStringLiteral("灵气"));
    QCOMPARE(store.terms().first().toMap().value(QStringLiteral("category")).toString(),
             QStringLiteral("修炼体系"));

    QVariantMap t2;
    t2[QStringLiteral("name")] = QStringLiteral("筑基");
    t2[QStringLiteral("content")] = QStringLiteral("修炼第一境界");
    int id2 = store.addTerm(t2);
    QVERIFY(id2 > id1);

    QVariantMap update;
    update[QStringLiteral("name")] = QStringLiteral("天地灵气");
    store.updateTerm(id1, update);
    QCOMPARE(store.terms().first().toMap().value(QStringLiteral("name")).toString(),
             QStringLiteral("天地灵气"));

    store.deleteTerm(id2);
    QCOMPARE(store.terms().size(), 1);
}

void TestEntityStore::knowledge_crud()
{
    QTemporaryDir tempDir;
    EntityStore store(tempDir.path());

    QVERIFY(store.knowledgeCards().isEmpty());

    QVariantMap k1;
    k1[QStringLiteral("name")] = QStringLiteral("世界设定");
    k1[QStringLiteral("content")] = QStringLiteral("这是一个修仙世界");
    k1[QStringLiteral("isGlobal")] = false;
    int id1 = store.addKnowledgeCard(k1);
    QVERIFY(id1 > 0);
    QCOMPARE(store.knowledgeCards().size(), 1);

    int globalBefore = EntityStore::globalKnowledgeCards().size();
    QVariantMap k2;
    k2[QStringLiteral("name")] = QStringLiteral("通用写作技巧");
    k2[QStringLiteral("content")] = QStringLiteral("三幕式结构");
    k2[QStringLiteral("isGlobal")] = true;
    int id2 = store.addKnowledgeCard(k2);
    QVERIFY(id2 > id1);
    QCOMPARE(store.knowledgeCards().size(), 2);
    QCOMPARE(EntityStore::globalKnowledgeCards().size(), globalBefore + 1);

    QVariantMap update;
    update[QStringLiteral("name")] = QStringLiteral("完整世界设定");
    store.updateKnowledgeCard(id1, update);
    QCOMPARE(store.knowledgeCards().first().toMap().value(QStringLiteral("name")).toString(),
             QStringLiteral("完整世界设定"));

    store.deleteKnowledgeCard(id2);
    QCOMPARE(store.knowledgeCards().size(), 1);
}

void TestEntityStore::memos_crud()
{
    QTemporaryDir tempDir;
    EntityStore store(tempDir.path());

    QVERIFY(store.memos().isEmpty());

    QVariantMap m1;
    m1[QStringLiteral("name")] = QStringLiteral("灵感记录");
    m1[QStringLiteral("content")] = QStringLiteral("第5章让主角遇到贵人");
    int id1 = store.addMemo(m1);
    QVERIFY(id1 > 0);
    QCOMPARE(store.memos().size(), 1);
    QCOMPARE(store.memos().first().toMap().value(QStringLiteral("name")).toString(),
             QStringLiteral("灵感记录"));

    QVariantMap m2;
    m2[QStringLiteral("name")] = QStringLiteral("伏笔");
    m2[QStringLiteral("content")] = QStringLiteral("老爷爷的真实身份");
    int id2 = store.addMemo(m2);
    QVERIFY(id2 > id1);

    QVariantMap update;
    update[QStringLiteral("content")] = QStringLiteral("第5章让主角遇到贵人，获得神器");
    store.updateMemo(id1, update);
    QCOMPARE(store.memos().first().toMap().value(QStringLiteral("content")).toString(),
             QStringLiteral("第5章让主角遇到贵人，获得神器"));

    store.deleteMemo(id2);
    QCOMPARE(store.memos().size(), 1);
}

void TestEntityStore::outlines_crud()
{
    QTemporaryDir tempDir;
    EntityStore store(tempDir.path());

    QVERIFY(store.outlines().isEmpty());

    QVariantMap o1;
    o1[QStringLiteral("name")] = QStringLiteral("主线大纲");
    o1[QStringLiteral("content")] = QStringLiteral("主角从凡人到仙帝");
    o1[QStringLiteral("type")] = QStringLiteral("main");
    int id1 = store.addOutline(o1);
    QVERIFY(id1 > 0);
    QCOMPARE(store.outlines().size(), 1);
    QCOMPARE(store.outlines().first().toMap().value(QStringLiteral("type")).toString(),
             QStringLiteral("main"));

    QVariantMap o2;
    o2[QStringLiteral("name")] = QStringLiteral("支线：感情线");
    o2[QStringLiteral("content")] = QStringLiteral("主角与女主的感情发展");
    o2[QStringLiteral("type")] = QStringLiteral("branch");
    int id2 = store.addOutline(o2);
    QVERIFY(id2 > id1);
    QCOMPARE(store.outlines().last().toMap().value(QStringLiteral("type")).toString(),
             QStringLiteral("branch"));

    QVariantMap update;
    update[QStringLiteral("name")] = QStringLiteral("主线：崛起之路");
    store.updateOutline(id1, update);
    QCOMPARE(store.outlines().first().toMap().value(QStringLiteral("name")).toString(),
             QStringLiteral("主线：崛起之路"));

    store.deleteOutline(id2);
    QCOMPARE(store.outlines().size(), 1);
}

void TestEntityStore::templates_crud()
{
    QVariantList original = EntityStore::globalTemplates();

    QVariantList empty;
    EntityStore::setGlobalTemplates(empty);
    QVERIFY(EntityStore::globalTemplates().isEmpty());

    QVariantMap t1;
    t1[QStringLiteral("name")] = QStringLiteral("保持原风");
    t1[QStringLiteral("content")] = QStringLiteral("请保持原文风格续写");
    t1[QStringLiteral("category")] = QStringLiteral("style");
    int id1 = EntityStore::addGlobalTemplate(t1);
    QVERIFY(id1 > 0);
    QCOMPARE(EntityStore::globalTemplates().size(), 1);
    QCOMPARE(EntityStore::globalTemplates().first().toMap().value(QStringLiteral("name")).toString(),
             QStringLiteral("保持原风"));
    QCOMPARE(EntityStore::globalTemplates().first().toMap().value(QStringLiteral("category")).toString(),
             QStringLiteral("style"));

    QVariantMap t2;
    t2[QStringLiteral("name")] = QStringLiteral("续写6.0");
    t2[QStringLiteral("content")] = QStringLiteral("续写要求模板");
    t2[QStringLiteral("category")] = QStringLiteral("requirement");
    int id2 = EntityStore::addGlobalTemplate(t2);
    QVERIFY(id2 > id1);

    QVariantMap update;
    update[QStringLiteral("content")] = QStringLiteral("请严格保持原文风格进行续写");
    EntityStore::updateGlobalTemplate(id1, update);
    QCOMPARE(EntityStore::globalTemplates().first().toMap().value(QStringLiteral("content")).toString(),
             QStringLiteral("请严格保持原文风格进行续写"));

    EntityStore::deleteGlobalTemplate(id2);
    QCOMPARE(EntityStore::globalTemplates().size(), 1);

    EntityStore::setGlobalTemplates(original);
}

void TestEntityStore::wordCountAutoCalc()
{
    QTemporaryDir tempDir;
    EntityStore store(tempDir.path());

    QVariantMap ch;
    ch[QStringLiteral("name")] = QStringLiteral("测试角色");
    ch[QStringLiteral("content")] = QStringLiteral("这是一段测试内容");
    int id = store.addCharacter(ch);
    QVariantMap saved = store.characters().first().toMap();
    QVERIFY(saved.contains(QStringLiteral("wordCount")));
    QCOMPARE(saved.value(QStringLiteral("wordCount")).toInt(), 8);

    QVariantMap update;
    update[QStringLiteral("content")] = QStringLiteral("这是修改后的内容，更长了");
    store.updateCharacter(id, update);
    QVariantMap updated = store.characters().first().toMap();
    QCOMPARE(updated.value(QStringLiteral("wordCount")).toInt(), 12);

    QVariantMap term;
    term[QStringLiteral("name")] = QStringLiteral("测试词条");
    term[QStringLiteral("content")] = QStringLiteral("一二三四五");
    int tid = store.addTerm(term);
    QCOMPARE(store.terms().first().toMap().value(QStringLiteral("wordCount")).toInt(), 5);
}

void TestEntityStore::emptyDirCreatesFiles()
{
    QTemporaryDir tempDir;
    QString bookDir = tempDir.path() + QStringLiteral("/newbook");
    QVERIFY(!QDir(bookDir).exists());

    EntityStore store(bookDir);
    QVERIFY(QDir(bookDir).exists());

    QVERIFY(QFile::exists(bookDir + QStringLiteral("/characters.json")));
    QVERIFY(QFile::exists(bookDir + QStringLiteral("/terms.json")));
    QVERIFY(QFile::exists(bookDir + QStringLiteral("/knowledge.json")));
    QVERIFY(QFile::exists(bookDir + QStringLiteral("/memos.json")));
    QVERIFY(QFile::exists(bookDir + QStringLiteral("/outlines.json")));

    QVERIFY(store.characters().isEmpty());
    QVERIFY(store.terms().isEmpty());
    QVERIFY(store.knowledgeCards().isEmpty());
    QVERIFY(store.memos().isEmpty());
    QVERIFY(store.outlines().isEmpty());
}

void TestEntityStore::persistenceRoundtrip()
{
    QTemporaryDir tempDir;
    QString bookDir = tempDir.path();

    {
        EntityStore store(bookDir);
        QVariantMap ch;
        ch[QStringLiteral("name")] = QStringLiteral("测试角色");
        ch[QStringLiteral("content")] = QStringLiteral("角色内容");
        ch[QStringLiteral("category")] = QStringLiteral("主角");
        store.addCharacter(ch);

        QVariantMap term;
        term[QStringLiteral("name")] = QStringLiteral("测试词条");
        term[QStringLiteral("content")] = QStringLiteral("词条内容");
        store.addTerm(term);
    }

    EntityStore store2(bookDir);
    QCOMPARE(store2.characters().size(), 1);
    QCOMPARE(store2.characters().first().toMap().value(QStringLiteral("name")).toString(),
             QStringLiteral("测试角色"));
    QCOMPARE(store2.terms().size(), 1);
    QCOMPARE(store2.terms().first().toMap().value(QStringLiteral("name")).toString(),
             QStringLiteral("测试词条"));
}

void TestEntityStore::seedTemplates()
{
    QVariantList original = EntityStore::globalTemplates();

    QVariantList empty;
    EntityStore::setGlobalTemplates(empty);
    QVERIFY(EntityStore::globalTemplates().isEmpty());

    EntityStore::seedTemplatesIfEmpty();
    QVariantList seeded = EntityStore::globalTemplates();
    QCOMPARE(seeded.size(), 5);

    int styleCount = 0;
    int reqCount = 0;
    QStringList styleNames;
    QStringList reqNames;
    for (const QVariant &v : seeded) {
        QVariantMap m = v.toMap();
        if (m.value(QStringLiteral("category")).toString() == QStringLiteral("style")) {
            ++styleCount;
            styleNames.append(m.value(QStringLiteral("name")).toString());
        } else if (m.value(QStringLiteral("category")).toString() == QStringLiteral("requirement")) {
            ++reqCount;
            reqNames.append(m.value(QStringLiteral("name")).toString());
        }
    }
    QCOMPARE(styleCount, 3);
    QCOMPARE(reqCount, 2);
    QVERIFY(styleNames.contains(QStringLiteral("保持原风")));
    QVERIFY(styleNames.contains(QStringLiteral("白话文")));
    QVERIFY(styleNames.contains(QStringLiteral("番茄爽文风")));
    QVERIFY(reqNames.contains(QStringLiteral("续写6.0")));
    QVERIFY(reqNames.contains(QStringLiteral("橙子一键续写")));

    EntityStore::seedTemplatesIfEmpty();
    QCOMPARE(EntityStore::globalTemplates().size(), 5);

    EntityStore::setGlobalTemplates(original);
}

QTEST_GUILESS_MAIN(TestEntityStore)
#include "test_entitystore.moc"
