#include "projectstore.h"

#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUuid>
#include <QDebug>

ProjectStore::ProjectStore(QObject *parent)
    : QObject(parent)
{
    QDir root(rootPath());
    if (!root.exists())
        root.mkpath(QStringLiteral("."));
}

QString ProjectStore::rootPath() const
{
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
           + QStringLiteral("/books");
}

QString ProjectStore::bookDir(const QString &id) const
{
    return rootPath() + QLatin1Char('/') + id;
}

QString ProjectStore::metaPath(const QString &id) const
{
    return bookDir(id) + QStringLiteral("/meta.json");
}

QVariantMap ProjectStore::readMeta(const QString &id) const
{
    QFile f(metaPath(id));
    if (!f.open(QIODevice::ReadOnly))
        return {};
    const QJsonDocument doc = QJsonDocument::fromJson(f.readAll());
    if (!doc.isObject())
        return {};
    return doc.toVariant().toMap();
}

void ProjectStore::writeMeta(const QString &id, const QVariantMap &book)
{
    QDir dir(bookDir(id));
    if (!dir.exists() && !dir.mkpath(QStringLiteral("."))) {
        qWarning() << "ProjectStore: 无法创建目录" << bookDir(id);
        return;
    }
    QVariantMap out = book;
    out[QStringLiteral("id")] = id;
    if (!out.contains(QStringLiteral("chapters")))
        out[QStringLiteral("chapters")] = QVariantList();

    const QJsonDocument doc(QJsonDocument::fromVariant(out));
    QFile f(metaPath(id));
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        qWarning() << "ProjectStore: 无法写入" << metaPath(id);
        return;
    }
    f.write(doc.toJson(QJsonDocument::Indented));
}

bool ProjectStore::exists(const QString &id) const
{
    return QFile::exists(metaPath(id));
}

QString ProjectStore::createBook(const QVariantMap &book)
{
    const QString id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    writeMeta(id, book);
    return id;
}

QVariantMap ProjectStore::loadBook(const QString &id) const
{
    return readMeta(id);
}

bool ProjectStore::saveBook(const QVariantMap &book) const
{
    const QString id = book.value(QStringLiteral("id")).toString();
    if (id.isEmpty() || !exists(id))
        return false;
    writeMeta(id, book);
    return true;
}

QVariantList ProjectStore::listBooks() const
{
    QVariantList result;
    QDir root(rootPath());
    if (!root.exists())
        return result;
    const QStringList dirs = root.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    for (const QString &d : dirs) {
        const QVariantMap m = readMeta(d);
        if (m.isEmpty())
            continue;
        QVariantMap card;
        card[QStringLiteral("id")] = d;
        card[QStringLiteral("title")] = m.value(QStringLiteral("title"));
        card[QStringLiteral("genreId")] = m.value(QStringLiteral("genreId"));
        card[QStringLiteral("genreName")] = m.value(QStringLiteral("genreName"));
        card[QStringLiteral("author")] = m.value(QStringLiteral("author"));
        card[QStringLiteral("hue")] = m.value(QStringLiteral("hue"));
        result.append(card);
    }
    return result;
}

QString ProjectStore::lastBookId() const
{
    QFile f(rootPath() + QStringLiteral("/.last"));
    if (!f.open(QIODevice::ReadOnly))
        return {};
    const QString id = QString::fromUtf8(f.readAll()).trimmed();
    return exists(id) ? id : QString();
}

void ProjectStore::setLastBookId(const QString &id)
{
    QDir(rootPath()).mkpath(QStringLiteral("."));
    QFile f(rootPath() + QStringLiteral("/.last"));
    if (f.open(QIODevice::WriteOnly | QIODevice::Truncate))
        f.write(id.toUtf8());
}
