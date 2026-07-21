#pragma once

#include <QDialog>
#include <QHeaderView>
#include <QSplitter>
#include <QTextEdit>
#include <QTreeView>
#include <QVBoxLayout>

namespace ui {

struct TypedefsDialog {
    QTreeView* tvtypedefs;
    QSplitter* splitter;
    QTextEdit* tecode;

    explicit TypedefsDialog(QDialog* self) {
        self->setAttribute(Qt::WA_DeleteOnClose);
        self->setWindowTitle("Type definitions");
        self->setFixedSize(1000, 600);

        this->tvtypedefs = new QTreeView();
        this->tvtypedefs->header()->setStretchLastSection(false);
        this->tvtypedefs->setUniformRowHeights(true);
        this->tvtypedefs->setRootIsDecorated(false);

        this->tecode = new QTextEdit();
        this->tecode->setLineWrapMode(QTextEdit::NoWrap);
        this->tecode->setUndoRedoEnabled(false);
        this->tecode->setReadOnly(true);

        this->splitter = new QSplitter(Qt::Horizontal);
        this->splitter->addWidget(this->tvtypedefs);
        this->splitter->addWidget(this->tecode);

        auto* vbox = new QVBoxLayout(self);
        vbox->addWidget(this->splitter);
    }
};

} // namespace ui
