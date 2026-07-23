#pragma once

#include <QByteArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QList>
#include <QString>

/**
 * 纯函数式 SSE（Server-Sent Events）解析器。
 *
 * 无网络依赖、无全局状态，可独立单测。负责把 OpenAI 兼容的流式响应
 * 字节流解析为正文增量（delta）。能够正确处理被 TCP 分包截断的行：
 * 未遇 '\n' 之前的数据会缓存在内部缓冲区，下次 feed() 时自动补全。
 */
class SseParser
{
public:
    /// 解析单行 SSE（形如 "data: {...json...}"）。
    /// 返回该行的正文增量；非 data 行 / "[DONE]" / 无 content 的 delta 返回空字符串。
    static QString parseLine(const QByteArray &line);

    /// 喂入任意分块字节，返回本次解析出的所有正文增量。
    QList<QString> feed(const QByteArray &chunk);

    /// 当前尚未消费完的缓冲区（用于错误体兜底解析）。
    const QByteArray &buffer() const { return m_buf; }

    /// 清空缓冲区（开始一次新请求时调用）。
    void reset() { m_buf.clear(); }

private:
    QByteArray m_buf;
};
