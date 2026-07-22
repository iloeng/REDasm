#pragma once

#include "models/typedefsfilter.h"
#include "support/syntaxhighlighter.h"
#include "ui/typedefsdialog.h"
#include <redasm/redasm.h>

class TypeDefSyntaxHighlighter: SyntaxHighlighter {
    Q_OBJECT

public:
    explicit TypeDefSyntaxHighlighter(QTextDocument* doc);
    void add_type(const QString& type);

private:
    QTextCharFormat m_typefmt;
};

class TypeDefsDialog: public QDialog {
    Q_OBJECT

public:
    explicit TypeDefsDialog(RDContext* ctx, QWidget* parent = nullptr);
    void generate_code(const QModelIndex& index);

private:
    ui::TypedefsDialog m_ui;
    TypedefsFilterModel* m_typedefsmodel;
    RDContext* m_context;
};
