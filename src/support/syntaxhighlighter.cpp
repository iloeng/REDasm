#include "syntaxhighlighter.h"

SyntaxHighlighter::SyntaxHighlighter(QTextDocument* parent)
    : QSyntaxHighlighter(parent) {}

void SyntaxHighlighter::add_word_rule(const QStringList& words,
                                      const QTextCharFormat& format) {
    if(words.isEmpty()) return;

    QStringList escaped;
    escaped.reserve(words.size());
    for(const QString& word : words)
        escaped.append(QRegularExpression::escape(word));

    const QString PATTERN =
        QStringLiteral("\\b(?:%1)\\b").arg(escaped.join(QLatin1Char('|')));
    m_rules.append({QRegularExpression(PATTERN), format});
}

void SyntaxHighlighter::add_symbol_rule(const QString& symbols,
                                        const QTextCharFormat& cf) {
    if(symbols.isEmpty()) return;

    QString char_class;
    char_class.reserve(symbols.size() * 2);
    for(const QChar& c : symbols)
        char_class += QRegularExpression::escape(QString(c));

    const QString PATTERN = QStringLiteral("[%1]").arg(char_class);
    m_rules.append({QRegularExpression(PATTERN), cf});
}

void SyntaxHighlighter::set_number_rule(const QTextCharFormat& cf,
                                        bool include_hex) {
    static const QString HEX_PART = QStringLiteral("\\b0[xX][0-9A-Fa-f]+\\b");
    static const QString DEC_PART =
        QStringLiteral("\\b[0-9]+(?:\\.[0-9]+)?\\b");

    const QString PATTERN =
        include_hex ? QStringLiteral("%1|%2").arg(HEX_PART, DEC_PART)
                    : DEC_PART;

    m_rules.append({QRegularExpression(PATTERN), cf});
}

void SyntaxHighlighter::add_regex_rule(const QRegularExpression& pattern,
                                       const QTextCharFormat& cf) {
    m_rules.append({pattern, cf});
}

void SyntaxHighlighter::set_line_comment_rule(const QString& start_token,
                                              const QTextCharFormat& cf) {
    m_commentpattern = QRegularExpression{
        QRegularExpression::escape(start_token) + QStringLiteral("[^\\n]*")};

    m_commentformat = cf;
    m_hascommentrule = true;
}

void SyntaxHighlighter::highlightBlock(const QString& text) {
    for(const HighlightingRule& rule : std::as_const(m_rules)) {
        QRegularExpressionMatchIterator it = rule.pattern.globalMatch(text);

        while(it.hasNext()) {
            const QRegularExpressionMatch MATCH = it.next();
            this->setFormat(MATCH.capturedStart(), MATCH.capturedLength(),
                            rule.format);
        }
    }

    if(m_hascommentrule) {
        const QRegularExpressionMatch MATCH = m_commentpattern.match(text);

        if(MATCH.hasMatch()) {
            this->setFormat(MATCH.capturedStart(), MATCH.capturedLength(),
                            m_commentformat);
        }
    }

    this->setCurrentBlockState(0);
}
