#pragma once

#include "Common.hpp"

#include <QTextObjectInterface>

#include <unordered_set>

class EmojiTextObject;

using Emoji = std::vector<unsigned int>;
using EmojiSet = std::unordered_set<Emoji>;

class EmojiTextObjectInterface : public QObject, public QTextObjectInterface
{
    Q_OBJECT
    Q_INTERFACES(QTextObjectInterface)

public:
    explicit EmojiTextObjectInterface(EmojiTextObject *emojiTextObject);

    void drawObject(QPainter *painter, const QRectF &rect, QTextDocument *document, int posInDocument, const QTextFormat &format) override;
    QSizeF intrinsicSize(QTextDocument *document, int posInDocument, const QTextFormat &format) override;

private:
    EmojiTextObject *m_emojiTextObject;
};

class EmojiTextObject : public QObject
{
    Q_OBJECT
public:
    explicit EmojiTextObject(QTextDocument *textDocument);
    ~EmojiTextObject() override = default;

    enum TextFormatProperty { EmojiUnicode = QTextFormat::UserProperty, EmojiImagePath, EmojiString };
    Q_ENUMS(TextFormatProperty)

    [[nodiscard]] static QString getEmojiString(const Emoji &emojiCode);
    [[nodiscard]] static QString getEmojiString(unsigned int character);

    QString getEmojiPath(const Emoji &code) const;
    bool hasEmoticon(const Emoji &code) const;
    QPixmap getEmoji(const Emoji &code, const QSize &size) const;

public slots:
    void onTextDocumentContentsChanged(int position, int charsRemoved, int charsAdded);

private:
    friend class EmojiTextObjectInterface;

    static const EmojiSet AllEmojis;
    static constexpr int EmojiTextFormatObjectType = QTextFormat::UserObject + 0xEEEE;

    [[nodiscard]] bool isEmoji(uint character) const;
    [[nodiscard]] bool isEmoji(const Emoji &emojiCode) const;

    [[nodiscard]] Emoji extractEmojiCode(QTextCursor cursor, int *selectionLength);

    [[nodiscard]] QString extractDocumentText(bool html = false) const;
    [[nodiscard]] QString extractCursorText(const QTextCursor &cursor, bool html = false) const;

    QByteArray svgPrepare(const QString &fileName) const;
    QPixmap svgToPixmap(const QString &fileName, const QSize &size) const;

    void applyTextCharFormat(int pos = 0);

    [[nodiscard]] int computeLineHeight(int posInDocument, const QTextFormat &format) const;

    QTextDocument *m_textDocument = nullptr;
    QSize m_minimumEmojiSize;

    quint8 m_maxEmojiCharCodeCount = 0;

    bool m_isApplyingFormats = false;
};

Q_DECLARE_METATYPE(Emoji)
