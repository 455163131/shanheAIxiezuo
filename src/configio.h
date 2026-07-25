#pragma once

#include <QString>
#include <QStringList>

class ConfigIo {
public:
    static bool exportConfig(const QString &filePath);
    static bool importConfig(const QString &filePath);

private:
    static const QStringList KEY_BLACKLIST;
};
