#include "sseparser.h"

QString SseParser::parseLine(const QByteArray &line)
{
    QByteArray l = line.trimmed();
    if (l.isEmpty())
        return {};
    if (!l.startsWith("data:"))
        return {};
    QByteArray data = l.mid(5).trimmed();
    if (data.isEmpty() || data == "[DONE]")
        return {};
    QJsonParseError perr;
    const QJsonDocument d = QJsonDocument::fromJson(data, &perr);
    if (perr.error != QJsonParseError::NoError || !d.isObject())
        return {};
    const QJsonObject obj = d.object();
    const QJsonArray choices = obj.value(QStringLiteral("choices")).toArray();
    if (choices.isEmpty())
        return {};
    const QJsonObject delta = choices.at(0).toObject().value(QStringLiteral("delta")).toObject();
    return delta.value(QStringLiteral("content")).toString();
}

QList<QString> SseParser::feed(const QByteArray &chunk)
{
    QList<QString> out;
    m_buf.append(chunk);
    int idx;
    while ((idx = m_buf.indexOf('\n')) != -1) {
        QByteArray line = m_buf.left(idx);
        m_buf.remove(0, idx + 1);
        const QString s = parseLine(line);
        if (!s.isEmpty())
            out.append(s);
    }
    return out;
}
