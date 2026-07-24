#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QIcon>
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QUrl>
#include <QQuickStyle>
#include <QPalette>
#include "bridge.h"

#ifdef Q_OS_WIN
#include <windows.h>
#endif

// 把所有 Qt 日志（含 QML 加载错误）写入 exe 同目录的 startup.log，方便本机排查
void shanheMessageHandler(QtMsgType type, const QMessageLogContext &, const QString &msg)
{
    static QFile f(QStringLiteral("startup.log"));
    static bool opened = f.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text);
    const char *tag = (type == QtFatalMsg)  ? "FATAL"
                    : (type == QtCriticalMsg) ? "CRIT"
                    : (type == QtWarningMsg) ? "WARN" : "INFO";
    if (opened) {
        QTextStream ts(&f);
        ts << QDateTime::currentDateTime().toString(QStringLiteral("hh:mm:ss"))
           << " [" << tag << "] " << msg << "\n";
        ts.flush();
    }
    fprintf(stderr, "[%s] %s\n", tag, qPrintable(msg));
}

int main(int argc, char *argv[])
{
    qInstallMessageHandler(shanheMessageHandler);

    // 强制使用 Fusion 风格（最可靠方式：在 QGuiApplication 构造前设置环境变量）。
    // Fusion 完整支持自定义 background，可彻底消除原生风格下的
    // "does not support customization of this control (background)" 警告。
    qputenv("QT_QUICK_CONTROLS_STYLE", "Fusion");

    QGuiApplication app(argc, argv);
    app.setApplicationName(QString::fromUtf8("山河AI写作"));
    app.setOrganizationName(QString::fromUtf8("ShanHe"));

    // 使用 Fusion 风格：它完整支持自定义 background，可彻底消除原生风格下
    // "does not support customization of this control (background)" 警告，
    // 并让 Slider / CheckBox / ScrollBar 等控件贴合暗色主题。
    QQuickStyle::setStyle(QStringLiteral("Fusion"));
    QPalette pal;
    pal.setColor(QPalette::Window,     QColor(0x16, 0x1b, 0x22));
    pal.setColor(QPalette::WindowText, QColor(0xe9, 0xe2, 0xcf));
    pal.setColor(QPalette::Base,      QColor(0x1b, 0x22, 0x30));
    pal.setColor(QPalette::Text,      QColor(0xe9, 0xe2, 0xcf));
    pal.setColor(QPalette::Button,    QColor(0x1b, 0x22, 0x30));
    pal.setColor(QPalette::ButtonText,QColor(0xe9, 0xe2, 0xcf));
    pal.setColor(QPalette::Highlight, QColor(0xca, 0xa8, 0x6a));
    app.setPalette(pal);

    QQmlApplicationEngine engine;

    // C++ 内核桥接：暴露为全局上下文属性 "ShanHe"
    ShanHeBridge bridge;
    engine.rootContext()->setContextProperty(QStringLiteral("ShanHe"), &bridge);

    // Bug-4：启动时检测 TLS 插件可用性，缺失则 emit tlsMissing 让 QML 设置页标红
    bridge.checkTlsOnStartup();

    QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed,
                     &app, [](const QUrl &url) {
        const QString err = QString::fromUtf8("QML 组件加载失败: ") + url.toString()
                          + QString::fromUtf8("\n请查看同目录 startup.log 获取详情。");
        qCritical() << err;
#ifdef Q_OS_WIN
        MessageBoxW(nullptr,
                    (LPCWSTR)err.utf16(),
                    (LPCWSTR)QString::fromUtf8("山河AI写作 · 启动失败").utf16(),
                    MB_ICONERROR | MB_OK);
#endif
        QCoreApplication::exit(-1);
    }, Qt::QueuedConnection);

    // main.qml 不在自动生成的 qmldir 中（qmldir 只暴露其他被 import 的类型），
    // 所以用 qrc URL 直接加载，规避 "Module contains no type named 'main'" 问题
    engine.load(QUrl(QStringLiteral("qrc:/qt/qml/ShanHe/qml/main.qml")));

    return app.exec();
}

