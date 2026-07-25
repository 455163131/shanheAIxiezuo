#pragma once

#include <QObject>
#include <QString>
#include <QVariantList>
#include <QVariantMap>

/**
 * 书籍持久化层（P2 质量基线：解决「重启即丢」；本轮升级为 10.2 完整契约拆分）。
 *
 * 存储布局（对齐 TECH_STACK.md 第 10.2 节契约——「同一份书数据，两种引擎可切换」）：
 *   <AppDataLocation>/books/<id>/
 *   ├── meta.json        # 桌面端元数据超集：{ schemaVersion, id, title,
 *   │                    #   genreId, genreName, author, hue, timeline,
 *   │                    #   direction, tone, hook, chaptersMeta:[标题…],
 *   │                    #   createdAt, updatedAt }
 *   ├── bible.md         # 世界设定圣经（worldView，逐行）
 *   ├── characters.json  # { "raw": "<人物卡自由文本>" }  —— 给 Python 引擎留结构入口
 *   ├── outline.json     # { "book": "<全书大纲>" }       —— 预留按章 {"1":…} 扩展
 *   ├── summaries.json   # { }                            —— 防失忆底（未来生成）
 *   ├── template.md      # {{var}} 提示词骨架（默认空）
 *   ├── chapters/
 *   │   ├── ch01.txt ... # 已写章节正文（1-based，两位补零）
 *   └── .runtime_memory.json  # 引擎运行期写，可不存在
 *
 * 以 QVariantMap 为中心（而非强类型 Book）：书籍是动态对象，且 QML 侧天然
 * 以 map 传递。Book 结构保留给未来的领域逻辑 / 单测。
 *
 * 向后兼容：旧版单一 meta.json（含内联 chapters 数组、无 schemaVersion）在
 * 首次 loadBook 时自动迁移为新目录布局，已存书籍不丢。
 *
 * 继承 QObject（仅用于对象树归属 / 内存管理：bridge 以 new ProjectStore(this) 托管），
 * 但无信号槽：仍是纯粹的文件读写工具，不引入额外构建耦合。
 */
class ProjectStore : public QObject
{
    Q_OBJECT
public:
    explicit ProjectStore(QObject *parent = nullptr);

    QString rootPath() const;
    QVariantList listBooks() const;
    QString createBook(const QVariantMap &book);
    QVariantMap loadBook(const QString &id) const;
    bool saveBook(const QVariantMap &book) const;
    bool exists(const QString &id) const;
    QString lastBookId() const;
    void setLastBookId(const QString &id);

    // 契约文件清单（供测试 / 调试断言目录布局）
    QString bookDir(const QString &id) const;
    QString biblePath(const QString &id) const;
    QString charactersPath(const QString &id) const;
    QString outlinePath(const QString &id) const;
    QString summariesPath(const QString &id) const;
    QString templatePath(const QString &id) const;
    QString chaptersDir(const QString &id) const;
    QString chapterPath(const QString &id, int n) const;

private:
    QString metaPath(const QString &id) const;

    void writeBookDir(const QString &id, const QVariantMap &book) const;
    QVariantMap readBookDir(const QString &id) const;
    void migrateIfNeeded(const QString &id) const;
    bool migrateV2toV3(const QString &id) const;
    void cleanupStaleChapters(const QString &id, int keepCount) const;

    static bool writeTextFile(const QString &path, const QString &text);
    static QString readTextFile(const QString &path);
    static bool writeJsonFile(const QString &path, const QVariant &v);
    static QVariant readJsonFile(const QString &path);
};
