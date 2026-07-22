#pragma once

#include "models/typedefsfilter.h"
#include "support/syntaxhighlighter.h"
#include "ui/typedefsdialog.h"
#include <redasm/redasm.h>

class TypedefSyntaxHighlighter: SyntaxHighlighter {
    Q_OBJECT

public:
    explicit TypedefSyntaxHighlighter(QTextDocument* doc);
    void add_type(const QString& type);

private:
    QTextCharFormat m_typefmt;
};

class TypedefsDialog: public QDialog {
    Q_OBJECT

public:
    explicit TypedefsDialog(RDContext* ctx, QWidget* parent = nullptr);
    void generate_code(const QModelIndex& index);

private:
    ui::TypedefsDialog m_ui;
    TypedefsFilterModel* m_typedefsmodel;
    RDContext* m_context;
};
