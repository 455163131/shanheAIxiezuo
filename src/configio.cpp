#include "configio.h"

#include <QSettings>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDateTime>

const QStringList ConfigIo::KEY_BLACKLIST = {
    QStringLiteral("api/key"),
    QStringLiteral("apiKey"),
};

bool ConfigIo::exportConfig(const QString &filePath)
{
    QSettings s("ShanHe", "ShanHeWriter");
    QJsonObject settingsObj;
    const auto keys = s.allKeys();
    for (const QString &key : keys) {
        if (KEY_BLACKLIST.contains(key, Qt::CaseInsensitive)) continue;
        settingsObj[key] = QJsonValue::fromVariant(s.value(key));
    }

    QJsonObject root;
    root["version"] = 1;
    root["settings"] = settingsObj;
    root["exportedAt"] = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);

    QJsonDocument doc(root);
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) return false;
    file.write(doc.toJson(QJsonDocument::Indented));
    return true;
}

bool ConfigIo::importConfig(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) return false;
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    if (!doc.isObject()) return false;
    QJsonObject root = doc.object();
    if (!root.contains("settings")) return false;
    QJsonObject settings = root["settings"].toObject();

    QSettings s("ShanHe", "ShanHeWriter");
    s.clear();
    for (auto it = settings.begin(); it != settings.end(); ++it) {
        if (KEY_BLACKLIST.contains(it.key(), Qt::CaseInsensitive)) continue;
        s.setValue(it.key(), it.value().toVariant());
    }
    return true;
}
