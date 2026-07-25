#include <QTest>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QFile>
#include <QDir>
#include <QSettings>
#include <QSslSocket>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDateTime>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QTemporaryDir>

#include "sseparser.h"
#include "book.h"
#include "bridge.h"
#include "httpllmclient.h"
#include "projectstore.h"
#include "personas.h"
#include "windowscredentialstore.h"
#include "fakellmclient.h"
#include "mockllmclient.h"
#include "llmparams.h"
#include "promptassembler.h"
#include "leakguard.h"
#include "textutils.h"
#include "configio.h"

class ShanHeTests : public QObject
{
    Q_OBJECT
private slots:
    void sseParser_singleLine();
    void sseParser_partialChunks();
    void sseParser_doneAndEmpty();
    void book_roundTrip();
    void bridge_streamsViaInjectedClient();
    void bridge_cancelSuppressesDone();
    void projectStore_roundTrip();
    void credentialStore_behaves();
    void bridge_decodeApiKeyDistinguishesFailure();
    void httpLlmClient_tlsCheck();
    void httpLlmClient_urlNormalization();
    void mockLlmClient_abortsImmediately();
    void mockLlmClient_generateWithControlBasicRound();
    void book_chapterSerializationRoundtrip();
    void book_bookPrefsRoundtrip();
    void projectStore_migrateV2toV3();
    void projectStore_importFromAiWritingDb();
    void configIo_exportImportRoundtrip();
};

void ShanHeTests::sseParser_singleLine()
{
    SseParser p;
    const QByteArray line = R"(data: {"choices":[{"delta":{"content":"你好"}}]})";
    const QList<QString> out = p.feed(line + "\n");
    QCOMPARE(out.size(), 1);
    QCOMPARE(out.first(), QStringLiteral("你好"));
}

void ShanHeTests::sseParser_partialChunks()
{
    SseParser p;
    // 一行被拆成多段 TCP 字节，验证跨分包缓冲正确
    const QByteArray a = R"(data: {"choices":[{"delta":{"content":)";
    const QByteArray b = R"("世界"}}]})";
    const QByteArray c = "\n";

    QList<QString> out;
    out += p.feed(a);
    out += p.feed(b);
    out += p.feed(c);

    QCOMPARE(out.size(), 1);
    QCOMPARE(out.first(), QStringLiteral("世界"));
}

void ShanHeTests::sseParser_doneAndEmpty()
{
    SseParser p;
    QList<QString> out;
    out += p.feed(QByteArray("data: [DONE]\n"));
    out += p.feed(QByteArray("data: {\"choices\":[{\"delta\":{\"role\":\"assistant\"}}]}\n"));
    // [DONE] 与无 content 的 delta 都不应产生正文增量
    QCOMPARE(out.size(), 0);
}

void ShanHeTests::book_roundTrip()
{
    Book b;
    b.m_id = QStringLiteral("b1");
    b.m_title = QStringLiteral("测试之书");
    b.m_genreId = QStringLiteral("g-x");
    Chapter c1;
    c1.m_title = QStringLiteral("第一章");
    c1.m_content = QStringLiteral("正文A");
    b.m_chapters.append(c1);

    const QJsonObject json = b.toJson();
    const Book back = Book::fromJson(json);

    QCOMPARE(back.m_id, b.m_id);
    QCOMPARE(back.m_title, b.m_title);
    QCOMPARE(back.m_genreId, b.m_genreId);
    QCOMPARE(back.m_chapters.size(), 1);
    QCOMPARE(back.m_chapters.first().m_content, QStringLiteral("正文A"));
}

void ShanHeTests::bridge_streamsViaInjectedClient()
{
    ShanHeBridge bridge;
    FakeLlmClient fake;
    fake.script = [](const QJsonObject &,
                     std::function<void(const QString &)> onChunk,
                     std::function<void(bool, const QString &)> onDone) {
        onChunk(QStringLiteral("你好"));
        onChunk(QStringLiteral("世界"));
        onDone(true, QString());
    };
    bridge.setLlmClient(&fake);

    QSignalSpy doneSpy(&bridge, &ShanHeBridge::generationDone);
    QSignalSpy chunkSpy(&bridge, &ShanHeBridge::generationChunk);

    bridge.generate(false, Personas::keys().at(0), QStringLiteral("写一个开头"));

    QCOMPARE(doneSpy.count(), 1);
    QCOMPARE(chunkSpy.count(), 2);
    QCOMPARE(doneSpy.first().first().toString(), QStringLiteral("你好世界"));
    QVERIFY(fake.streamChatCalls >= 1);
}

void ShanHeTests::bridge_cancelSuppressesDone()
{
    ShanHeBridge bridge;
    FakeLlmClient fake;

    // 脚本只捕获 onDone 回调，不立即发射，模拟「生成进行中」
    std::function<void(bool, const QString &)> capDone;
    fake.script = [&](const QJsonObject &,
                      std::function<void(const QString &)>,
                      std::function<void(bool, const QString &)> onDone) {
        capDone = onDone;
    };
    bridge.setLlmClient(&fake);

    QSignalSpy doneSpy(&bridge, &ShanHeBridge::generationDone);
    QSignalSpy errSpy(&bridge, &ShanHeBridge::error);

    bridge.generate(false, Personas::keys().at(0), QStringLiteral("x"));
    QVERIFY(capDone);                      // 生成已开始，回调已捕获

    bridge.stopGeneration();              // 设置 m_cancelled 并通知客户端 abort

    // 模拟网络在取消之后才返回 finished（abort 触发的 canceled 回调）
    capDone(false, QStringLiteral("cancelled"));

    // P0-1 验证：取消后不应误 emit generationDone / error
    QCOMPARE(doneSpy.count(), 0);
    QCOMPARE(errSpy.count(), 0);
    QVERIFY(fake.aborted);
}

void ShanHeTests::projectStore_roundTrip()
{
    // 让 AppDataLocation 指向临时目录，避免污染真实用户数据
    QStandardPaths::setTestModeEnabled(true);
    ProjectStore store;
    // Test isolation: remove leftover books from previous runs so books.size()==1 holds.
    QDir(store.rootPath()).removeRecursively();

    QVariantMap b;
    b[QStringLiteral("title")] = QStringLiteral("测试书");
    b[QStringLiteral("genreId")] = QStringLiteral("m_x");
    b[QStringLiteral("genreName")] = QStringLiteral("玄幻");
    b[QStringLiteral("author")] = QStringLiteral("某人");
    b[QStringLiteral("hue")] = QStringLiteral("#fff");
    b[QStringLiteral("worldView")] = QStringLiteral("世界观内容");
    // 开新书引导访谈新增字段（增量持久化，向后兼容）
    b[QStringLiteral("direction")] = QStringLiteral("男频");
    b[QStringLiteral("tone")]      = QStringLiteral("热血燃、悬疑烧脑");
    b[QStringLiteral("hook")]      = QStringLiteral("开局获得神秘系统");
    QVariantList ch;
    QVariantMap c1;
    c1[QStringLiteral("title")] = QStringLiteral("第1章");
    c1[QStringLiteral("content")] = QStringLiteral("正文A");
    ch.append(c1);
    b[QStringLiteral("chapters")] = ch;

    const QString id = store.createBook(b);
    QVERIFY(!id.isEmpty());

    // 验证 10.2 契约目录布局已生成（worldView→bible.md、characters→characters.json、
    // outline→outline.json、第 N 章→chapters/chNN.txt）
    QVERIFY(QFile::exists(store.bookDir(id)));
    QVERIFY(QFile::exists(store.biblePath(id)));
    QVERIFY(QFile::exists(store.charactersPath(id)));
    QVERIFY(QFile::exists(store.outlinePath(id)));
    QVERIFY(QFile::exists(store.chapterPath(id, 1)));

    QVariantMap loaded = store.loadBook(id);
    QCOMPARE(loaded[QStringLiteral("title")].toString(), QStringLiteral("测试书"));
    QCOMPARE(loaded[QStringLiteral("worldView")].toString(), QStringLiteral("世界观内容"));
    // 验证引导访谈答案随书持久化（关闭重开仍可用）
    QCOMPARE(loaded[QStringLiteral("direction")].toString(), QStringLiteral("男频"));
    QCOMPARE(loaded[QStringLiteral("tone")].toString(), QStringLiteral("热血燃、悬疑烧脑"));
    QCOMPARE(loaded[QStringLiteral("hook")].toString(), QStringLiteral("开局获得神秘系统"));
    QVariantList chLoaded = loaded[QStringLiteral("chapters")].toList();
    QCOMPARE(chLoaded.size(), 1);
    QCOMPARE(chLoaded.first().toMap()[QStringLiteral("content")].toString(), QStringLiteral("正文A"));

    // 修改章节正文后保存，验证回写
    QVariantMap b2 = loaded;
    QVariantList ch2 = b2[QStringLiteral("chapters")].toList();
    QVariantMap c2 = ch2.first().toMap();
    c2[QStringLiteral("content")] = QStringLiteral("改后正文");
    ch2[0] = c2;
    b2[QStringLiteral("chapters")] = ch2;
    QVERIFY(store.saveBook(b2));

    QVariantMap re = store.loadBook(id);
    QCOMPARE(re[QStringLiteral("chapters")].toList().first().toMap()[QStringLiteral("content")].toString(),
            QStringLiteral("改后正文"));

    // 书架列表应能看到该书
    QVariantList books = store.listBooks();
    QCOMPARE(books.size(), 1);
    QCOMPARE(books.first().toMap()[QStringLiteral("title")].toString(), QStringLiteral("测试书"));

    // last book id
    store.setLastBookId(id);
    QCOMPARE(store.lastBookId(), id);
}

void ShanHeTests::credentialStore_behaves()
{
    // Windows 上应可用 DPAPI，做真实 round-trip；
    // 非 Windows 应 unavailable()，验证优雅降级（不崩溃、返回 false）。
    if (WindowsCredentialStore::available()) {
        QByteArray blob;
        QVERIFY(WindowsCredentialStore::protect(QStringLiteral("sk-1234567890abcdef"), blob));
        QVERIFY(!blob.isEmpty());
        QString out;
        QVERIFY(WindowsCredentialStore::unprotect(blob, out));
        QCOMPARE(out, QStringLiteral("sk-1234567890abcdef"));
    } else {
        QVERIFY(!WindowsCredentialStore::available());
        QByteArray blob;
        QString out;
        QVERIFY(!WindowsCredentialStore::protect(QStringLiteral("x"), blob));
        QVERIFY(!WindowsCredentialStore::unprotect(blob, out));
    }
}

void ShanHeTests::bridge_decodeApiKeyDistinguishesFailure()
{
    // 清理可能残留的 api/key，确保情况 1（未配置）从干净状态开始
    {
        QSettings s(QStringLiteral("ShanHe"), QStringLiteral("ShanHeWriter"));
        s.remove(QStringLiteral("api/key"));
        s.sync();
    }

    ShanHeBridge bridge;
    // 情况 1：未配置 -> std::nullopt
    auto result1 = bridge.decodeApiKey();
    QVERIFY(!result1.has_value());

    // 情况 2：配置了有效 key -> 有值
    bridge.saveConfig(QStringLiteral("https://api.example.com/v1"),
                      QStringLiteral("sk-valid-key"),
                      QStringLiteral("gpt-4o"), 0.8,
                      QStringLiteral("api"));
    auto result2 = bridge.decodeApiKey();
    QVERIFY(result2.has_value());
    QCOMPARE(result2.value(), QStringLiteral("sk-valid-key"));

    // 情况 3：解密失败 -> std::nullopt + emit error 信号
    QSignalSpy spy(&bridge, &ShanHeBridge::error);
    bridge.injectCorruptedApiKeyForTest();
    auto result3 = bridge.decodeApiKey();
    QVERIFY(!result3.has_value());
    QCOMPARE(spy.count(), 1);
    QVERIFY(spy.takeFirst().at(0).toString().contains(
        QStringLiteral("解密失败"), Qt::CaseInsensitive));
}

void ShanHeTests::httpLlmClient_tlsCheck()
{
    // Bug-4：启动时检测 TLS 插件可用性，缺失时 emit tlsMissing 让 QML 设置页标红。
    // 构造函数不自动调用 checkTlsOnStartup，由 main.cpp 显式触发，避免构造期 emit
    // 让 QSignalSpy 错过信号。
    const bool supports = HttpLlmClient::checkTlsAvailable();
    QCOMPARE(supports, QSslSocket::supportsSsl());

    ShanHeBridge bridge;
    QSignalSpy spy(&bridge, &ShanHeBridge::tlsMissing);
    bridge.checkTlsOnStartup();
    if (supports) {
        QCOMPARE(spy.count(), 0);
    } else {
        QCOMPARE(spy.count(), 1);
    }
}

void ShanHeTests::httpLlmClient_urlNormalization()
{
    // 7.7: URL normalization regression guard.
    HttpLlmClient client;

    client.configure("https://api.example.com/v1/", "sk-test");
    QCOMPARE(client.normalizedChatUrlForTest(),
             QStringLiteral("https://api.example.com/v1/chat/completions"));

    client.configure("https://api.example.com/v1", "sk-test");
    QCOMPARE(client.normalizedChatUrlForTest(),
             QStringLiteral("https://api.example.com/v1/chat/completions"));

    client.configure("https://api.example.com/v1/chat/completions", "sk-test");
    QCOMPARE(client.normalizedChatUrlForTest(),
             QStringLiteral("https://api.example.com/v1/chat/completions"));

    client.configure("https://api.example.com", "sk-test");
    QCOMPARE(client.normalizedChatUrlForTest(),
             QStringLiteral("https://api.example.com/chat/completions"));
}

void ShanHeTests::mockLlmClient_abortsImmediately()
{
    // Bug-7: abort() must stop timer, call onDone(false), isStreaming() returns false.
    MockLlmClient mock;
    QVERIFY(!mock.isStreaming());

    QJsonObject payload;
    payload["messages"] = QJsonArray{};
    payload["stream"] = true;
    bool gotChunk = false;
    bool gotDone = false;
    bool doneOk = true;
    mock.streamChat(payload,
        [&](const QString &) { gotChunk = true; },
        [&](bool ok, const QString &) { gotDone = true; doneOk = ok; });

    QVERIFY(mock.isStreaming());
    mock.abort();
    QVERIFY(!mock.isStreaming());

    QTest::qWait(200);
    QVERIFY(!gotChunk || !mock.isStreaming());
    // abort must notify onDone(false, "aborted") so upstream UI can finalize state.
    QVERIFY2(gotDone, "abort should call onDone so callers can finalize state");
    QVERIFY2(!doneOk, "abort onDone should report ok=false");
}

void ShanHeTests::mockLlmClient_generateWithControlBasicRound()
{
    // Task 13: verify generateWithControl state machine callback order via mock.
    // The mock emits onMeta(stream_wait) -> onDelta * N -> onMeta(stream_done) -> onDone.
    // HttpLlmClient's real state machine (streamOnce + needsContinue + LeakGuard +
    // classifyError + timeout) depends on a live QNetworkReply, so we cover the
    // callback contract here and rely on integration for the network path.

    MockLlmClient mock;

    ILlmClient::GenConfig cfg;
    cfg.wordCountMin = 100;
    cfg.wordCountMax = 200;
    cfg.maxTokens = 2000;
    cfg.systemMessage = QStringLiteral("你是网文续写助手");
    cfg.userMessage = QStringLiteral("请续写");
    cfg.jobId = QStringLiteral("test_job_1");

    LlmParams::SamplingFields sampling;
    sampling.temperature = 0.9;
    cfg.sampling = sampling;

    QString lastPhase;
    QString fullOutput;
    bool doneCalled = false;
    bool doneOk = false;
    ILlmClient::GenResult doneResult;

    ILlmClient::GenCallbacks cb;
    cb.onDelta = [&](const QString &delta) { fullOutput += delta; };
    cb.onThinking = [&](const QString &) {};
    cb.onMeta = [&](const QString &phase) { lastPhase = phase; };
    cb.onDone = [&](bool ok, const QString &err, const QString &full,
                    const ILlmClient::GenResult &result) {
        Q_UNUSED(err)
        doneCalled = true;
        doneOk = ok;
        fullOutput = full;
        doneResult = result;
    };

    mock.generateWithControl(cfg, cb, QStringLiteral("test_job_1"));

    // Job should be streaming immediately after start.
    QVERIFY(mock.isJobStreaming(QStringLiteral("test_job_1")));

    // Mock timer ticks every 50ms producing ~5 chars per tick; scripted text is
    // under 100 chars, so 2s is plenty for completion.
    QTest::qWait(2000);

    QVERIFY(doneCalled);
    QVERIFY(doneOk);
    QVERIFY(!fullOutput.isEmpty());
    QVERIFY(doneResult.wordCount >= 0);
    QVERIFY(!mock.isJobStreaming(QStringLiteral("test_job_1")));
}

void ShanHeTests::book_chapterSerializationRoundtrip()
{
    Chapter ch;
    ch.m_title = QStringLiteral("第1章 觉醒");
    ch.m_content = QStringLiteral("这是第一章正文。");
    ch.m_summary = QStringLiteral("本章讲了主角觉醒系统。");
    ch.m_summarySource = QStringLiteral("manual");
    ch.m_sortOrder = 0;
    ch.m_status = QStringLiteral("draft");
    ch.m_wordCount = 10;
    ch.m_aiConfig = QVariantMap{
        {"apiKeyId", 1},
        {"modelName", "deepseek-chat"},
        {"storyBackground", "背景设定"},
        {"chapterPlot", "细纲"},
        {"styleTemplateId", 1},
        {"creativityIndex", 3},
        {"thinkingAuto", true},
        {"thinkingIndex", 2},
        {"wordCountMin", 2000},
        {"wordCountMax", 2500},
        {"recentMode", "lastN"},
        {"recentValue", 2000},
        {"emptyPolicy", "placeholder"},
    };
    ch.m_linkedCharacters = QVector<int>{1, 3, 5};
    ch.m_linkedTerms = QVector<int>{2, 4};
    ch.m_linkedKnowledge = QVector<int>{1};
    ch.m_linkedMemos = QVector<int>{1, 2};
    ch.m_linkedOutlines = QVector<int>{1};

    QJsonObject obj = ch.toJson();
    Chapter ch2 = Chapter::fromJson(obj);
    QCOMPARE(ch2.m_title, ch.m_title);
    QCOMPARE(ch2.m_content, ch.m_content);
    QCOMPARE(ch2.m_summary, ch.m_summary);
    QCOMPARE(ch2.m_summarySource, ch.m_summarySource);
    QCOMPARE(ch2.m_wordCount, ch.m_wordCount);
    QCOMPARE(ch2.m_aiConfig["modelName"].toString(), QStringLiteral("deepseek-chat"));
    QCOMPARE(ch2.m_aiConfig["creativityIndex"].toInt(), 3);
    QCOMPARE(ch2.m_linkedCharacters.size(), 3);
    QCOMPARE(ch2.m_linkedCharacters[0], 1);
    QCOMPARE(ch2.m_linkedTerms.size(), 2);
    QCOMPARE(ch2.m_linkedKnowledge.size(), 1);
    QCOMPARE(ch2.m_linkedMemos.size(), 2);
    QCOMPARE(ch2.m_linkedOutlines.size(), 1);
}

void ShanHeTests::book_bookPrefsRoundtrip()
{
    Book book;
    book.m_id = "test-id";
    book.m_title = "测试书";
    book.m_prefs = QVariantMap{
        {"creativityIndex", 3},
        {"thinkingAuto", true},
        {"thinkingIndex", 2},
        {"wordCountMin", 2000},
        {"wordCountMax", 2500},
        {"recentMode", "lastN"},
        {"recentValue", 2000},
    };

    QJsonObject meta = book.toMetaJson();
    Book book2 = Book::fromMetaJson(meta);
    QCOMPARE(book2.m_prefs["creativityIndex"].toInt(), 3);
    QCOMPARE(book2.m_prefs["thinkingAuto"].toBool(), true);
    QCOMPARE(book2.m_prefs["recentMode"].toString(), QStringLiteral("lastN"));
    QCOMPARE(book2.m_prefs["recentValue"].toInt(), 2000);
}


void ShanHeTests::configIo_exportImportRoundtrip()
{
    QSettings s("ShanHe", "ShanHeWriter");
    s.setValue("api/base", "https://api.example.com/v1");
    s.setValue("api/model", "gpt-4o");
    s.setValue("api/backend", "api");
    s.setValue("ui/skin", "novel");
    s.setValue("ui/dark", true);
    s.setValue("gen/creativityIndex", 3);
    s.setValue("gen/wordCountMin", 2000);
    s.setValue("gen/wordCountMax", 2500);

    QString tmpPath = QDir::tempPath() + "/shanhe_config_test.json";
    QFile::remove(tmpPath);

    bool exportOk = ConfigIo::exportConfig(tmpPath);
    QVERIFY(exportOk);

    QFile file(tmpPath);
    QVERIFY(file.open(QIODevice::ReadOnly));
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    QVERIFY(doc.isObject());
    QJsonObject obj = doc.object();
    QVERIFY(obj.contains("settings"));
    QJsonObject settings = obj["settings"].toObject();
    QVERIFY(!settings.contains("api/key"));
    QCOMPARE(settings["api/base"].toString(), QStringLiteral("https://api.example.com/v1"));
    QCOMPARE(settings["ui/skin"].toString(), QStringLiteral("novel"));

    s.clear();
    bool importOk = ConfigIo::importConfig(tmpPath);
    QVERIFY(importOk);
    QCOMPARE(s.value("api/base").toString(), QStringLiteral("https://api.example.com/v1"));
    QCOMPARE(s.value("api/model").toString(), QStringLiteral("gpt-4o"));
    QCOMPARE(s.value("ui/skin").toString(), QStringLiteral("novel"));
    QCOMPARE(s.value("gen/creativityIndex").toInt(), 3);
    QVERIFY(s.value("api/key").isNull());

    s.clear();
    QFile::remove(tmpPath);
}


void ShanHeTests::projectStore_migrateV2toV3()
{
    // Use test mode to isolate data directory
    QStandardPaths::setTestModeEnabled(true);
    ProjectStore store;

    // Clean up any leftover books
    QDir(store.rootPath()).removeRecursively();
    QDir().mkpath(store.rootPath());

    // Create a v2 book directory
    QString bookId = "test-book";
    QString bookDir = store.bookDir(bookId);
    QDir().mkpath(bookDir);
    QDir().mkpath(bookDir + "/chapters");

    // v2 meta.json - chaptersMeta is array of strings in v2
    QJsonObject meta;
    meta["schemaVersion"] = 2;
    meta["id"] = bookId;
    meta["title"] = "测试书";
    QJsonArray chaptersMeta;
    chaptersMeta.append("第1章");
    chaptersMeta.append("第2章");
    meta["chaptersMeta"] = chaptersMeta;
    QFile metaFile(bookDir + "/meta.json");
    metaFile.open(QIODevice::WriteOnly);
    metaFile.write(QJsonDocument(meta).toJson());
    metaFile.close();

    // v2 characters.json (old raw format)
    QJsonObject chars;
    chars["raw"] = "主角：张三，18岁，修仙者。";
    QFile charsFile(bookDir + "/characters.json");
    charsFile.open(QIODevice::WriteOnly);
    charsFile.write(QJsonDocument(chars).toJson());
    charsFile.close();

    // Helper lambda to write a text file
    auto writeTextFile = [](const QString &path, const QString &text) {
        QFile f(path);
        f.open(QIODevice::WriteOnly | QIODevice::Truncate);
        f.write(text.toUtf8());
        f.close();
    };

    // v2 chapter text files
    writeTextFile(bookDir + "/chapters/ch01.txt", "第一章正文。");
    writeTextFile(bookDir + "/chapters/ch02.txt", "第二章正文。");

    // Empty summaries.json
    writeTextFile(bookDir + "/summaries.json", "{}");

    // Also create required v2 files that writeBookDir normally creates
    writeTextFile(bookDir + "/bible.md", "");
    writeTextFile(bookDir + "/outline.json", "{\"book\":\"\"}");
    writeTextFile(bookDir + "/template.md", "");

    // Run migration (triggered by loadBook)
    QVariantMap loadedBook = store.loadBook(bookId);
    QVERIFY(!loadedBook.isEmpty());

    // Read meta.json directly to verify schemaVersion and prefs
    QFile metaFile2(bookDir + "/meta.json");
    metaFile2.open(QIODevice::ReadOnly);
    QJsonObject metaObj = QJsonDocument::fromJson(metaFile2.readAll()).object();
    QCOMPARE(metaObj["schemaVersion"].toInt(), 3);
    QVERIFY(metaObj.contains("prefs"));
    QJsonObject prefs = metaObj["prefs"].toObject();
    QCOMPARE(prefs["creativityIndex"].toInt(), 3);
    QCOMPARE(prefs["thinkingAuto"].toBool(), true);
    QCOMPARE(prefs["wordCountMin"].toInt(), 2000);
    QCOMPARE(prefs["wordCountMax"].toInt(), 2500);
    QCOMPARE(prefs["recentMode"].toString(), QStringLiteral("lastN"));

    // Verify chapter meta files created
    QVERIFY(QFile::exists(bookDir + "/chapters/ch01.meta.json"));
    QVERIFY(QFile::exists(bookDir + "/chapters/ch02.meta.json"));

    // Verify ch01.meta.json content
    QFile ch01MetaFile(bookDir + "/chapters/ch01.meta.json");
    ch01MetaFile.open(QIODevice::ReadOnly);
    QJsonObject ch01Meta = QJsonDocument::fromJson(ch01MetaFile.readAll()).object();
    QCOMPARE(ch01Meta["title"].toString(), QStringLiteral("第1章"));

    // Verify characters upgraded: raw preserved + items array with the raw content
    QFile charsFile2(bookDir + "/characters.json");
    charsFile2.open(QIODevice::ReadOnly);
    QJsonObject chars2 = QJsonDocument::fromJson(charsFile2.readAll()).object();
    QCOMPARE(chars2["schemaVersion"].toInt(), 1);
    QVERIFY(chars2.contains("items"));
    QCOMPARE(chars2["items"].toArray().size(), 1);
    QCOMPARE(chars2["raw"].toString(), QStringLiteral("主角：张三，18岁，修仙者。"));
    QCOMPARE(chars2["items"].toArray()[0].toObject()["description"].toString(),
             QStringLiteral("主角：张三，18岁，修仙者。"));

    // Verify empty json files created for other entities
    QVERIFY(QFile::exists(bookDir + "/terms.json"));
    QVERIFY(QFile::exists(bookDir + "/knowledge.json"));
    QVERIFY(QFile::exists(bookDir + "/memos.json"));
    QVERIFY(QFile::exists(bookDir + "/outlines.json"));

    // Verify entity files have correct schema
    QFile termsFile(bookDir + "/terms.json");
    termsFile.open(QIODevice::ReadOnly);
    QJsonObject termsObj = QJsonDocument::fromJson(termsFile.readAll()).object();
    QCOMPARE(termsObj["schemaVersion"].toInt(), 1);
    QVERIFY(termsObj.contains("items"));
    QCOMPARE(termsObj["items"].toArray().size(), 0);

    // Clean up
    QDir(store.rootPath()).removeRecursively();
}



void ShanHeTests::projectStore_importFromAiWritingDb()
{
    // Skip if SQLite driver not available
    if (!QSqlDatabase::isDriverAvailable("QSQLITE")) QSKIP("QSQLITE not available");

    QTemporaryDir tmpDir;
    QString booksDir = tmpDir.path() + "/books";
    QDir().mkpath(booksDir);

    // Create a minimal project1-style ai_writing.db
    QString dbPath = tmpDir.path() + "/ai_writing.db";
    {
        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "import_test");
        db.setDatabaseName(dbPath);
        QVERIFY(db.open());

        QSqlQuery q(db);
        q.exec("CREATE TABLE novels (id INTEGER PRIMARY KEY, title TEXT, style TEXT)");
        q.exec("INSERT INTO novels (id, title, style) VALUES (1, '测试小说', '番茄爽文风')");

        q.exec("CREATE TABLE chapters (id INTEGER PRIMARY KEY, novel_id INTEGER, title TEXT, content TEXT, summary TEXT, sort_order INTEGER, word_count INTEGER)");
        q.exec("INSERT INTO chapters VALUES (1, 1, '第1章 觉醒', '第一章正文。', '第一章摘要', 1, 6)");
        q.exec("INSERT INTO chapters VALUES (2, 1, '第2章 修真', '第二章正文。', '第二章摘要', 2, 6)");

        q.exec("CREATE TABLE characters (id INTEGER PRIMARY KEY, novel_id INTEGER, name TEXT, description TEXT, folder_id INTEGER, sort_order INTEGER, pinned INTEGER, hidden INTEGER)");
        q.exec("INSERT INTO characters VALUES (1, 1, '张三', '18岁修仙者', 1, 1, 0, 0)");

        q.exec("CREATE TABLE character_folders (id INTEGER PRIMARY KEY, novel_id INTEGER, name TEXT, sort_order INTEGER)");
        q.exec("INSERT INTO character_folders VALUES (1, 1, '主角阵营', 1)");

        q.exec("CREATE TABLE terms (id INTEGER PRIMARY KEY, novel_id INTEGER, name TEXT, content TEXT, category TEXT)");
        q.exec("INSERT INTO terms VALUES (1, 1, '筑基期', '修仙基础阶段', '修炼境界')");

        q.exec("CREATE TABLE knowledge_cards (id INTEGER PRIMARY KEY, novel_id INTEGER, title TEXT, content TEXT, category TEXT, is_global INTEGER)");
        q.exec("INSERT INTO knowledge_cards VALUES (1, 1, '修仙体系', '筑基-金丹-元婴', '修炼体系', 0)");

        q.exec("CREATE TABLE memos (id INTEGER PRIMARY KEY, novel_id INTEGER, title TEXT, content TEXT)");
        q.exec("INSERT INTO memos VALUES (1, 1, '注意', '主角金手指别太早')");

        q.exec("CREATE TABLE outlines (id INTEGER PRIMARY KEY, novel_id INTEGER, title TEXT, content TEXT, type TEXT)");
        q.exec("INSERT INTO outlines VALUES (1, 1, '主线大纲', '第1章 觉醒 -> 第2章 修真', 'main')");

        q.exec("CREATE TABLE templates (id INTEGER PRIMARY KEY, type TEXT, title TEXT, content TEXT)");
        q.exec("INSERT INTO templates VALUES (1, 'style', '老模板', '旧风格内容')");

        db.close();
    }
    QSqlDatabase::removeDatabase("import_test");

    // Import
    bool ok = ProjectStore::importFromAiWritingDb(dbPath, booksDir);
    QVERIFY(ok);

    // Verify book created - use listBooks with a custom root path
    // We need to check the directory directly since ProjectStore uses AppDataLocation
    QDir booksDirObj(booksDir);
    QStringList bookDirs = booksDirObj.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    QCOMPARE(bookDirs.size(), 1);

    QString newBookId = bookDirs[0];
    QString newBookDir = booksDir + "/" + newBookId;
    QVERIFY(QDir(newBookDir).exists());

    // Verify meta.json
    QFile metaFile(newBookDir + "/meta.json");
    QVERIFY(metaFile.open(QIODevice::ReadOnly));
    QJsonObject meta = QJsonDocument::fromJson(metaFile.readAll()).object();
    QCOMPARE(meta["title"].toString(), QStringLiteral("测试小说"));
    QCOMPARE(meta["schemaVersion"].toInt(), 3);

    // Verify chapter text + meta
    QVERIFY(QFile::exists(newBookDir + "/chapters/ch01.txt"));
    QVERIFY(QFile::exists(newBookDir + "/chapters/ch01.meta.json"));
    QFile meta1(newBookDir + "/chapters/ch01.meta.json");
    meta1.open(QIODevice::ReadOnly);
    QJsonObject ch1 = QJsonDocument::fromJson(meta1.readAll()).object();
    QCOMPARE(ch1["title"].toString(), QStringLiteral("第1章 觉醒"));
    QCOMPARE(ch1["summary"].toString(), QStringLiteral("第一章摘要"));

    // Verify characters
    QFile charsFile(newBookDir + "/characters.json");
    charsFile.open(QIODevice::ReadOnly);
    QJsonObject chars = QJsonDocument::fromJson(charsFile.readAll()).object();
    QCOMPARE(chars["items"].toArray().size(), 1);
    QCOMPARE(chars["items"][0].toObject()["name"].toString(), QStringLiteral("张三"));
    QCOMPARE(chars["folders"].toArray().size(), 1);

    // Verify other entities exist
    QVERIFY(QFile::exists(newBookDir + "/terms.json"));
    QVERIFY(QFile::exists(newBookDir + "/knowledge.json"));
    QVERIFY(QFile::exists(newBookDir + "/memos.json"));
    QVERIFY(QFile::exists(newBookDir + "/outlines.json"));
}

QTEST_GUILESS_MAIN(ShanHeTests)
#include "test_shanhe.moc"
