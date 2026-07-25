#pragma once

#include <QString>
#include <QVariantList>
#include <QVariantMap>

// EntityStore: JSON file persistence for 6 entity types.
// Each book has its own directory with characters.json, terms.json,
// knowledge.json, memos.json, outlines.json.
// Templates and global knowledge are stored in AppDataLocation (shared across books).
class EntityStore
{
public:
    explicit EntityStore(const QString &bookDir);

    // Characters
    QVariantList characters() const;
    void setCharacters(const QVariantList &chars);
    int addCharacter(const QVariantMap &charData);
    void updateCharacter(int id, const QVariantMap &charData);
    void deleteCharacter(int id);
    QVariantList characterFolders() const;
    void setCharacterFolders(const QVariantList &folders);

    // Terms
    QVariantList terms() const;
    void setTerms(const QVariantList &terms);
    int addTerm(const QVariantMap &termData);
    void updateTerm(int id, const QVariantMap &termData);
    void deleteTerm(int id);

    // Knowledge
    QVariantList knowledgeCards() const;
    void setKnowledgeCards(const QVariantList &cards);
    int addKnowledgeCard(const QVariantMap &cardData);
    void updateKnowledgeCard(int id, const QVariantMap &cardData);
    void deleteKnowledgeCard(int id);

    // Memos
    QVariantList memos() const;
    void setMemos(const QVariantList &memos);
    int addMemo(const QVariantMap &memoData);
    void updateMemo(int id, const QVariantMap &memoData);
    void deleteMemo(int id);

    // Outlines
    QVariantList outlines() const;
    void setOutlines(const QVariantList &outlines);
    int addOutline(const QVariantMap &outlineData);
    void updateOutline(int id, const QVariantMap &outlineData);
    void deleteOutline(int id);

    // Templates (global, shared across books)
    static QVariantList globalTemplates();
    static void setGlobalTemplates(const QVariantList &templates);
    static int addGlobalTemplate(const QVariantMap &tmplData);
    static void updateGlobalTemplate(int id, const QVariantMap &tmplData);
    static void deleteGlobalTemplate(int id);
    static void seedTemplatesIfEmpty();

    // Global knowledge (isGlobal=true cards)
    static QVariantList globalKnowledgeCards();
    static void addGlobalKnowledge(const QVariantMap &cardData);

private:
    QString m_bookDir;
    void ensureDir();
    static QString globalTemplatesPath();
    static QString globalKnowledgePath();
    static int nextId(const QVariantList &items);
    static int autoWordCount(const QString &content);
    QVariantList loadJsonList(const QString &filename, const QString &key) const;
    void saveJsonList(const QString &filename, const QString &key, const QVariantList &items) const;
    QVariantList loadJsonFolders(const QString &filename) const;
    void saveJsonWithFolders(const QString &filename, const QVariantList &items, const QVariantList &folders) const;
    static QVariantList loadGlobalJsonList(const QString &path, const QString &key);
    static void saveGlobalJsonList(const QString &path, const QString &key, const QVariantList &items);
};
