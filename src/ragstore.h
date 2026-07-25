#pragma once

#include <QString>
#include <QStringList>
#include <QList>
#include <QVector>
#include <QVariantList>
#include <QHash>

// RAG 检索增强存储层的一篇文档（章节分块/实体/术语/知识/大纲/备忘录）
// vector 字段在 TF-IDF 模式下为该文档的 TF-IDF 向量，在 Embedding 模式下为 API 返回的向量
struct RagDocument {
    qlonglong id = 0;
    QString novelId;
    QString docType;     // "chapter" / "character" / "term" / "knowledge" / "outline" / "memo"
    QString docId;       // 对应实体/章节的 ID
    QString title;       // 标题
    QString content;     // 原始内容文本
    QString chunk;       // 分块后的文本片段（chapter 类型会分块）
    int chunkIndex = 0;  // 块索引
    QVector<float> vector;  // TF-IDF 向量或 embedding 向量
    qint64 createdAt = 0;
};

// RAG 向量检索数据层
// 双模式架构：
//   1. TF-IDF 模式（默认，纯本地）：中文 bigram + 英文/数字分词，TF-IDF + 余弦相似度
//   2. Embedding 模式（预留，需要 API）：调 embedding API 生成向量存 SQLite（暂未实现）
// 存储后端：SQLite 优先，JSON 文件 fallback
class RagStore {
public:
    struct SearchResult {
        qlonglong docId = 0;
        QString docType;
        QString title;
        QString chunk;
        float score = 0.0f;
    };

    static RagStore &instance();

    bool hasSqlite() const;
    QString storageMode() const;  // "tfidf" / "embedding"

    // 索引管理
    // 索引一篇文档；对 chapter 类型会按段落分块（默认 500 字），返回首块 ID
    qlonglong indexDocument(const QString &novelId, const QString &docType,
                            const QString &docId, const QString &title,
                            const QString &content, int chunkSize = 500);
    // 索引章节：默认按 500 字分块
    void indexChapter(const QString &novelId, const QString &chapterId,
                      const QString &title, const QString &content);
    // 索引实体：不分块，整篇存
    void indexEntity(const QString &novelId, const QString &docType,
                     const QString &docId, const QString &title,
                     const QString &content);
    bool removeDocument(qlonglong id);
    bool removeByDocId(const QString &novelId, const QString &docType, const QString &docId);
    void clearNovel(const QString &novelId);

    // 检索：按 query 文本对 novelId 下的文档做 TF-IDF 余弦相似度排序，返回 topK 条
    QList<SearchResult> search(const QString &novelId, const QString &query,
                               int topK = 5, const QStringList &docTypeFilter = {});

    // 重建某小说的全部索引向量（IDF 漂移后调用）
    void rebuildIndex(const QString &novelId);

    // 统计
    int documentCount(const QString &novelId);
    int documentCount(const QString &novelId, const QString &docType);

    // ---- TF-IDF 工具方法（暴露以便单元测试）----
    // 中文按字 bigram（"沈青云" → "沈青","青云","云"）；英文/数字按非字母数字字符切分
    QStringList tokenize(const QString &text) const;
    // 余弦相似度；向量维度不一致或零向量返回 0
    float cosineSimilarity(const QVector<float> &a, const QVector<float> &b) const;

private:
    RagStore();
    bool m_hasSqlite;
    QString m_jsonPath;
    QVariantList m_jsonDocs;

    void initSqlite();
    void loadJson();
    void saveJson();
    static QString jsonPath();

    // TF-IDF 内部实现
    QVector<float> computeTfidfVector(const QString &novelId, const QString &text);
    QHash<QString, int> buildVocabulary(const QString &novelId);
    double computeIdf(const QString &term, const QString &novelId);

    // 章节分块：按段落边界累积，不超过 chunkSize 字；chunkSize<=0 时不分块
    QStringList chunkText(const QString &content, int chunkSize) const;
};
