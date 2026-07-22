#pragma once

#include <QList>
#include <QRegularExpression>
#include <QSyntaxHighlighter>
#include <QTextCharFormat>

class SyntaxHighlighter: public QSyntaxHighlighter {
    Q_OBJECT

public:
    explicit SyntaxHighlighter(QTextDocument* parent = nullptr);
    void add_word_rule(const QStringList& words, const QTextCharFormat& cf);
    void add_symbol_rule(const QString& symbols, const QTextCharFormat& cf);
    void set_number_rule(const QTextCharFormat& cf, bool include_hex = true);
    void add_regex_rule(const QRegularExpression& pattern,
                        const QTextCharFormat& cf);
    void set_line_comment_rule(const QString& start_token,
                               const QTextCharFormat& cf);

    void add_word_fg(const QStringList& words, const QColor& c) {
        QTextCharFormat cf;
        cf.setForeground(c);
        this->add_word_rule(words, cf);
    }

    void add_symbol_fg(const QString& symbols, const QColor& c) {
        QTextCharFormat cf;
        cf.setForeground(c);
        this->add_symbol_rule(symbols, cf);
    }

    void set_number_fg(const QColor& c, bool include_hex = true) {
        QTextCharFormat cf;
        cf.setForeground(c);
        this->set_number_rule(cf, include_hex);
    }

    void add_regex_fg(const QRegularExpression& pattern, const QColor& c) {
        QTextCharFormat cf;
        cf.setForeground(c);
        this->add_regex_rule(pattern, cf);
    }

    void set_line_comment_fg(const QString& start_token, const QColor& c) {
        QTextCharFormat cf;
        cf.setForeground(c);
        this->set_line_comment_rule(start_token, cf);
    }

protected:
    void highlightBlock(const QString& text) override;

private:
    struct HighlightingRule {
        QRegularExpression pattern;
        QTextCharFormat format;
    };

    QList<HighlightingRule> m_rules;
    QRegularExpression m_commentpattern;
    QTextCharFormat m_commentformat;
    bool m_hascommentrule = false;
};
