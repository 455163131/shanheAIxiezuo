#include "consistencychecker.h"

#include <QRegularExpression>
#include <QSet>
#include <algorithm>

namespace {
// 角色名字附近的性别代词扫描窗口大小（字符数）
constexpr int PRONOUN_WINDOW = 24;
// 错别字近似匹配阈值：相似度 >= 此值 且 < 1.0 视为疑似错别字
constexpr double TYPO_SIMILARITY_THRESHOLD = 0.5;

// 性别 → 期望代词；冲突代词。
struct GenderPronoun {
    QString expected;   // 设定性别对应的代词
    QString conflicting; // 与设定冲突的代词
};

GenderPronoun pronounFor(const QString &gender)
{
    // 规范化：取首字即可（"男"/"女"）
    const QChar g = gender.trimmed().isEmpty() ? QChar()
                                                : gender.trimmed().at(0);
    if (g == QChar::fromLatin1('M') || g == QChar::fromLatin1('m')
        || g == QStringLiteral("男").at(0)) {
        return {QStringLiteral("他"), QStringLiteral("她")};
    }
    if (g == QChar::fromLatin1('F') || g == QChar::fromLatin1('f')
        || g == QStringLiteral("女").at(0)) {
        return {QStringLiteral("她"), QStringLiteral("他")};
    }
    // 其他/未设定：不报性别冲突
    return {QString(), QString()};
}

// 时间表达正则：覆盖测试用例所需的常见词
const QRegularExpression &timelineRegex()
{
    static const QRegularExpression re(
        QStringLiteral(
            "(昨天|昨日|今天|今日|明天|明日|后天|后日|前天|前日|大后天|大前天|次日|翌日|"
            "(?:\\d+|[一二三四五六七八九十两半几])天前|"
            "(?:\\d+|[一二三四五六七八九十两半几])天后|"
            "次年|翌年|明年|今年|去年|前年|"
            "(?:\\d+|[一二三四五六七八九十两])年前|"
            "(?:\\d+|[一二三四五六七八九十两])年后|"
            "数月后|数月前|片刻后|随即|旋即|稍后|不久|随后)"));
    return re;
}

// 古风 vs 现代词表（极简版，仅作示例检查）
const QVector<QPair<QString, QString>> &archaicModernPairs()
{
    // first: 古风词, second: 现代词
    static const QVector<QPair<QString, QString>> pairs = {
        {QStringLiteral("之"), QStringLiteral("的")},
        {QStringLiteral("曰"), QStringLiteral("说")},
        {QStringLiteral("吾"), QStringLiteral("我")},
        {QStringLiteral("尔"), QStringLiteral("你")},
        {QStringLiteral("乃"), QStringLiteral("是")},
        {QStringLiteral("且"), QStringLiteral("而且")},
        {QStringLiteral("皆"), QStringLiteral("都")},
    };
    return pairs;
}
} // namespace

QList<int> ConsistencyChecker::findAllOccurrences(const QString &text, const QString &name)
{
    QList<int> positions;
    if (name.isEmpty() || text.isEmpty())
        return positions;
    int from = 0;
    while (true) {
        int idx = text.indexOf(name, from, Qt::CaseSensitive);
        if (idx < 0)
            break;
        positions.append(idx);
        from = idx + name.size();
    }
    return positions;
}

QString ConsistencyChecker::windowAround(const QString &text, int pos, int windowSize)
{
    if (text.isEmpty() || pos < 0 || pos >= text.size())
        return QString();
    int start = std::max(0, pos - windowSize);
    int end = static_cast<int>(std::min(text.size(), qsizetype(pos + windowSize)));
    return text.mid(start, end - start);
}

double ConsistencyChecker::similarity(const QString &a, const QString &b)
{
    if (a.isEmpty() && b.isEmpty())
        return 1.0;
    const int la = a.size();
    const int lb = b.size();
    if (la == 0 || lb == 0)
        return 0.0;

    // 标准编辑距离（Levenshtein）
    QVector<QVector<int>> dp(la + 1, QVector<int>(lb + 1, 0));
    for (int i = 0; i <= la; ++i)
        dp[i][0] = i;
    for (int j = 0; j <= lb; ++j)
        dp[0][j] = j;
    for (int i = 1; i <= la; ++i) {
        for (int j = 1; j <= lb; ++j) {
            const int cost = (a.at(i - 1) == b.at(j - 1)) ? 0 : 1;
            dp[i][j] = std::min({dp[i - 1][j] + 1,
                                  dp[i][j - 1] + 1,
                                  dp[i - 1][j - 1] + cost});
        }
    }
    const int dist = dp[la][lb];
    const int maxLen = std::max(la, lb);
    return 1.0 - static_cast<double>(dist) / maxLen;
}

// ============== 1. 角色一致性 ==============
QList<ConsistencyIssue> ConsistencyChecker::checkCharacterConsistency(const ConsistencyInput &input)
{
    QList<ConsistencyIssue> issues;
    if (input.chapterText.isEmpty())
        return issues;

    for (const EntityRef &c : input.characters) {
        if (c.name.isEmpty())
            continue;

        // (a) 角色根本没在正文出现：不报（避免噪音）
        const QList<int> occurrences = findAllOccurrences(input.chapterText, c.name);
        if (occurrences.isEmpty())
            continue;

        // (b) 性别代词冲突
        const GenderPronoun gp = pronounFor(c.gender);
        if (!gp.expected.isEmpty() && !gp.conflicting.isEmpty()) {
            for (int pos : occurrences) {
                const QString win = windowAround(input.chapterText, pos, PRONOUN_WINDOW);
                if (win.indexOf(gp.conflicting) >= 0) {
                    ConsistencyIssue is;
                    is.type = QStringLiteral("character");
                    is.severity = QStringLiteral("warning");
                    is.title = QStringLiteral("角色性别代词与设定冲突");
                    is.detail = QStringLiteral("角色「%1」设定为「%2」，但正文附近使用了代词「%3」。")
                                    .arg(c.name, c.gender, gp.conflicting);
                    is.suggestion = QStringLiteral("将代词改为「%1」以匹配角色性别设定。").arg(gp.expected);
                    is.location = input.chapterId;
                    is.evidence = win;
                    issues.append(is);
                    break; // 同一角色只报一次，避免重复噪音
                }
            }
        }

        // (c) 错别字近似匹配：扫描正文中是否存在与角色名高度相似但不同的字符串
        if (c.name.size() >= 2) {
            const int len = c.name.size();
            QSet<QString> reported;
            for (int i = 0; i + len <= input.chapterText.size(); ++i) {
                const QString candidate = input.chapterText.mid(i, len);
                if (candidate == c.name)
                    continue;
                // 必须与角色名相邻：候选至少有一个字符与角色名相同位置相同
                bool adjacent = false;
                for (int k = 0; k < len; ++k) {
                    if (candidate.at(k) == c.name.at(k)) {
                        adjacent = true;
                        break;
                    }
                }
                if (!adjacent)
                    continue;
                const double sim = similarity(candidate, c.name);
                if (sim >= TYPO_SIMILARITY_THRESHOLD && sim < 1.0 && !reported.contains(candidate)) {
                    reported.insert(candidate);
                    ConsistencyIssue is;
                    is.type = QStringLiteral("character");
                    is.severity = QStringLiteral("warning");
                    is.title = QStringLiteral("疑似角色名错别字");
                    is.detail = QStringLiteral("正文中出现「%1」，与角色「%2」高度相似，可能是错别字。")
                                    .arg(candidate, c.name);
                    is.suggestion = QStringLiteral("核对角色名拼写是否正确。");
                    is.location = input.chapterId;
                    is.evidence = candidate;
                    issues.append(is);
                }
            }
        }
    }
    return issues;
}

// ============== 2. 时间线一致性 ==============
QList<ConsistencyIssue> ConsistencyChecker::checkTimelineConsistency(const ConsistencyInput &input)
{
    QList<ConsistencyIssue> issues;
    const QString text = input.chapterText;
    if (text.isEmpty())
        return issues;

    // 提取所有时间表达（按出现顺序）
    struct TimeHit {
        QString token;
        int pos;
    };
    QList<TimeHit> hits;
    QRegularExpressionMatchIterator it = timelineRegex().globalMatch(text);
    while (it.hasNext()) {
        QRegularExpressionMatch m = it.next();
        hits.append({m.captured(0), static_cast<int>(m.capturedStart())});
    }
    if (hits.size() < 2)
        return issues;

    // 检测模式：同一事件用矛盾时间描述
    // 简化规则：「昨天」+「次日」描述同一相邻事件，视为矛盾
    //            「明天」+「次日」也算
    //            「三年后」+「两年前」时间倒流
    for (int i = 0; i < hits.size(); ++i) {
        for (int j = i + 1; j < hits.size(); ++j) {
            const QString &a = hits[i].token;
            const QString &b = hits[j].token;
            // 同一日相邻事件冲突
            if ((a == QStringLiteral("昨天") && b == QStringLiteral("次日"))
                || (a == QStringLiteral("昨日") && b == QStringLiteral("次日"))
                || (a == QStringLiteral("明天") && b == QStringLiteral("次日"))
                || (a == QStringLiteral("明日") && b == QStringLiteral("次日"))
                || (a == QStringLiteral("次日") && b == QStringLiteral("昨天"))
                || (a == QStringLiteral("次日") && b == QStringLiteral("昨日"))) {
                ConsistencyIssue is;
                is.type = QStringLiteral("timeline");
                is.severity = QStringLiteral("error");
                is.title = QStringLiteral("同一事件的时间描述前后矛盾");
                is.detail = QStringLiteral("前文出现「%1」，后文出现「%2」，二者指向同一相邻事件但方向相反。")
                                .arg(a, b);
                is.suggestion = QStringLiteral("统一为单一时间描述。");
                is.location = input.chapterId;
                is.evidence = QStringLiteral("%1 ... %2").arg(a, b);
                issues.append(is);
                return issues; // 同类问题只报一次
            }
            // 时间倒流：N 年后 -> N 年前（且后者 N <= 前者）
            static const QRegularExpression afterRe(
                QStringLiteral("^(\\d+|[一二三四五六七八九十两])年后$"));
            static const QRegularExpression beforeRe(
                QStringLiteral("^(\\d+|[一二三四五六七八九十两])年前$"));
            QRegularExpressionMatch ma = afterRe.match(a);
            QRegularExpressionMatch mb = beforeRe.match(b);
            if (ma.hasMatch() && mb.hasMatch()) {
                bool okA = false, okB = false;
                int na = ma.captured(1).toInt(&okA);
                int nb = mb.captured(1).toInt(&okB);
                if (!okA) na = 0;
                if (!okB) nb = 0;
                // 若前者 >= 后者，疑似时间倒流
                if (na >= nb) {
                    ConsistencyIssue is;
                    is.type = QStringLiteral("timeline");
                    is.severity = QStringLiteral("error");
                    is.title = QStringLiteral("时间线倒流");
                    is.detail = QStringLiteral("前文出现「%1」，后文出现「%2」，可能存在时间倒流。")
                                    .arg(a, b);
                    is.suggestion = QStringLiteral("确认时间顺序，必要时改为回忆叙述。");
                    is.location = input.chapterId;
                    is.evidence = QStringLiteral("%1 ... %2").arg(a, b);
                    issues.append(is);
                    return issues;
                }
            }
        }
    }
    return issues;
}

// ============== 3. 词条一致性 ==============
QList<ConsistencyIssue> ConsistencyChecker::checkTermConsistency(const ConsistencyInput &input)
{
    QList<ConsistencyIssue> issues;
    if (input.chapterText.isEmpty())
        return issues;

    for (const EntityRef &t : input.terms) {
        if (t.name.isEmpty())
            continue;
        // 词条必须出现在正文中
        const QList<int> occurrences = findAllOccurrences(input.chapterText, t.name);
        if (occurrences.isEmpty())
            continue;

        // 从 summary 中提取关键属性：
        //   方向：东方/西方/南方/北方/东边/西边/南边/北边
        static const QRegularExpression directionRe(
            QStringLiteral("(东方|西方|南方|北方|东边|西边|南边|北边|东侧|西侧|南侧|北侧)"));
        QRegularExpressionMatch dm = directionRe.match(t.summary);
        if (dm.hasMatch()) {
            const QString definedDir = dm.captured(1);
            // 在正文里检查词条名附近是否出现与定义相反的方向
            for (int pos : occurrences) {
                const QString win = windowAround(input.chapterText, pos, 40);
                QRegularExpressionMatch wm = directionRe.match(win);
                if (wm.hasMatch()) {
                    const QString usedDir = wm.captured(1);
                    // 反方向判定：只比较首字
                    const QChar defCh = definedDir.at(0);
                    const QChar useCh = usedDir.at(0);
                    static const QHash<QChar, QChar> opposites = {
                        {QStringLiteral("东").at(0), QStringLiteral("西").at(0)},
                        {QStringLiteral("西").at(0), QStringLiteral("东").at(0)},
                        {QStringLiteral("南").at(0), QStringLiteral("北").at(0)},
                        {QStringLiteral("北").at(0), QStringLiteral("南").at(0)},
                    };
                    if (opposites.value(defCh) == useCh) {
                        ConsistencyIssue is;
                        is.type = QStringLiteral("term");
                        is.severity = QStringLiteral("warning");
                        is.title = QStringLiteral("词条定义与正文用法矛盾");
                        is.detail = QStringLiteral("词条「%1」定义为「%2」，但正文写为「%3」。")
                                        .arg(t.name, definedDir, usedDir);
                        is.suggestion = QStringLiteral("按词条定义修正方向描述。");
                        is.location = input.chapterId;
                        is.evidence = win;
                        issues.append(is);
                        break;
                    }
                }
            }
        }
    }
    return issues;
}

// ============== 4. 伏笔追踪 ==============
QList<ConsistencyIssue> ConsistencyChecker::checkPlotConsistency(const ConsistencyInput &input)
{
    QList<ConsistencyIssue> issues;
    if (input.chapterText.isEmpty())
        return issues;

    // 大纲中标记的伏笔/承诺/悬念
    static const QRegularExpression foreshadowRe(
        QStringLiteral("(伏笔|承诺|悬念)\\s*[:：]\\s*(.+)"));
    for (const EntityRef &o : input.outlines) {
        if (o.name.isEmpty() && o.content.isEmpty())
            continue;
        const QString haystack = o.content.isEmpty() ? o.name : o.content;
        // 在大纲标题或内容里搜索伏笔标记
        QRegularExpressionMatchIterator it = foreshadowRe.globalMatch(haystack);
        while (it.hasNext()) {
            QRegularExpressionMatch m = it.next();
            const QString kind = m.captured(1);
            const QString token = m.captured(2).trimmed();
            if (token.isEmpty())
                continue;

            // 在正文（含 previousText）中检查是否被回收
            const QString fullText = input.previousText + QStringLiteral("\n") + input.chapterText;
            if (fullText.indexOf(token) >= 0)
                continue; // 已回收

            ConsistencyIssue is;
            is.type = QStringLiteral("plot");
            is.severity = QStringLiteral("info");
            is.title = QStringLiteral("伏笔/承诺未回收");
            is.detail = QStringLiteral("大纲「%1」中标记的%2：「%3」在正文中未提及。")
                            .arg(o.name, kind, token);
            is.suggestion = QStringLiteral("考虑在后续章节回收该%1，或显式标记为已放弃。").arg(kind);
            is.location = input.chapterId;
            is.evidence = token;
            issues.append(is);
        }
    }
    return issues;
}

// ============== 5. 风格一致性 ==============
QList<ConsistencyIssue> ConsistencyChecker::checkStyleConsistency(const ConsistencyInput &input)
{
    QList<ConsistencyIssue> issues;
    const QString text = input.chapterText;
    if (text.isEmpty())
        return issues;

    // (a) 人称混用：第一人称「我」与第三人称「他/她」同时出现
    //    排除引号内对话：极简实现——只看是否两者频次都显著
    bool hasFirst = text.indexOf(QStringLiteral("我")) >= 0;
    bool hasThird = text.indexOf(QStringLiteral("他")) >= 0
                    || text.indexOf(QStringLiteral("她")) >= 0;
    if (hasFirst && hasThird) {
        // 进一步粗略排除：要求"我"出现次数 >= 3 且 第三人称 >= 3，避免单次对话误报
        const int firstCount = text.count(QStringLiteral("我"));
        const int thirdCount = text.count(QStringLiteral("他")) + text.count(QStringLiteral("她"));
        if (firstCount >= 3 && thirdCount >= 3) {
            ConsistencyIssue is;
            is.type = QStringLiteral("style");
            is.severity = QStringLiteral("warning");
            is.title = QStringLiteral("人称混用");
            is.detail = QStringLiteral("正文同时大量出现第一人称「我」（%1 次）和第三人称「他/她」（%2 次），可能存在人称混乱。")
                            .arg(firstCount).arg(thirdCount);
            is.suggestion = QStringLiteral("统一为第一人称或第三人称叙述视角。");
            is.location = input.chapterId;
            is.evidence = QStringLiteral("我×%1 他/她×%2").arg(firstCount).arg(thirdCount);
            issues.append(is);
        }
    }

    // (b) 古风 / 现代词汇混用：简单词表匹配
    bool hasArchaic = false;
    bool hasModern = false;
    QString archaicHit, modernHit;
    for (const auto &pair : archaicModernPairs()) {
        if (!hasArchaic && text.indexOf(pair.first) >= 0) {
            hasArchaic = true;
            archaicHit = pair.first;
        }
        if (!hasModern && text.indexOf(pair.second) >= 0) {
            hasModern = true;
            modernHit = pair.second;
        }
        if (hasArchaic && hasModern)
            break;
    }
    if (hasArchaic && hasModern) {
        ConsistencyIssue is;
        is.type = QStringLiteral("style");
        is.severity = QStringLiteral("warning");
        is.title = QStringLiteral("古风与现代词汇混用");
        is.detail = QStringLiteral("正文同时出现古风词「%1」和现代词「%2」，文风可能不一致。")
                        .arg(archaicHit, modernHit);
        is.suggestion = QStringLiteral("统一文风：要么纯古风，要么纯现代。");
        is.location = input.chapterId;
        is.evidence = QStringLiteral("%1 ... %2").arg(archaicHit, modernHit);
        issues.append(is);
    }
    return issues;
}

// ============== 综合检查 ==============
QList<ConsistencyIssue> ConsistencyChecker::checkAll(const ConsistencyInput &input)
{
    QList<ConsistencyIssue> all;
    all.append(checkCharacterConsistency(input));
    all.append(checkTimelineConsistency(input));
    all.append(checkTermConsistency(input));
    all.append(checkPlotConsistency(input));
    all.append(checkStyleConsistency(input));
    return all;
}
