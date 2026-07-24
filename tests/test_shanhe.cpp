#include <QTest>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QFile>
#include <QSettings>
#include <QSslSocket>

#include "sseparser.h"
#include "book.h"
#include "bridge.h"
#include "httpllmclient.h"
#include "projectstore.h"
#include "personas.h"
#include "windowscredentialstore.h"
#include "fakellmclient.h"
#include "mockllmclient.h"

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
    void mockLlmClient_abortsImmediately();
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

QTEST_GUILESS_MAIN(ShanHeTests)
#include "test_shanhe.moc"
