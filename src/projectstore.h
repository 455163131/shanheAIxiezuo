#pragma once

#include <QObject>
#include <QString>
#include <QVariantList>
#include <QVariantMap>

/**
 * 书籍持久化层（P2 质量基线：解决「重启即丢」）。
 *
 * 存储布局（对齐 TECH_STACK.md 第 10.2 节契约的简化版）：
 *   <AppDataLocation>/books/<id>/meta.json
 *   meta.json = { id, title, genreId, genreName, author, hue,
 *                 worldView, characters, timeline, outlineText,
 *                 chapters: [ {title, content}, ... ] }
 *
 * 以 QVariantMap 为中心（而非强类型 Book）：书籍是动态对象，且 QML 侧
 * 天然以 map 传递，省去一层转换。Book 结构保留给未来的领域逻辑 / 单测。
 *
 * 不继承 Q_OBJECT / 无信号槽：纯粹的文件读写工具，降低构建耦合。
 */
class ProjectStore
{
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

private:
    QString bookDir(const QString &id) const;
    QString metaPath(const QString &id) const;
    QVariantMap readMeta(const QString &id) const;
    void writeMeta(const QString &id, const QVariantMap &book);
};
