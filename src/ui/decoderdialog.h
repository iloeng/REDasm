#pragma once

#include "support/themeprovider.h"
#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHexView/qhexview.h>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSplitter>
#include <QTabWidget>
#include <QVBoxLayout>

namespace ui {

struct DecoderDialog {
    QComboBox* cbprocessors;
    QLineEdit* leaddress;
    QTabWidget* tabwidget;
    QPushButton* pbclear;

    QHexView* dec_hexview;
    QPlainTextEdit* dec_textedit;
    QCheckBox* dec_cbxdetails;
    QDialogButtonBox* dec_buttonbox;

    QHexView* enc_hexview;
    QPlainTextEdit* enc_textedit;
    QLabel* enc_lblerror;
    QDialogButtonBox* enc_buttonbox;

    explicit DecoderDialog(QDialog* self) {
        self->setAttribute(Qt::WA_DeleteOnClose);
        self->setWindowTitle("Instructions decoder/encoder");
        self->resize(700, 500);

        this->cbprocessors = new QComboBox();
        this->leaddress = new QLineEdit();
        this->pbclear = new QPushButton("Clear");
        this->dec_cbxdetails = new QCheckBox("Show details");

        auto* grid = new QGridLayout();
        grid->setColumnStretch(1, 1);
        grid->addWidget(new QLabel("Processors:"), 0, 0, Qt::AlignRight);
        grid->addWidget(this->cbprocessors, 0, 1, 1, 2);
        grid->addWidget(this->pbclear, 0, 3);
        grid->addWidget(new QLabel("Address:"), 1, 0, Qt::AlignRight);
        grid->addWidget(this->leaddress, 1, 1, 1, 3);

        this->tabwidget = new QTabWidget();
        this->tabwidget->addTab(this->create_decoder_tab(self), "Decoder");
        this->tabwidget->addTab(this->create_encoder_tab(self), "Encoder");

        auto* vbox = new QVBoxLayout(self);
        vbox->addLayout(grid);
        vbox->addWidget(this->tabwidget, 1);
    }

private:
    QGroupBox* group_widget(QWidget* w, const QString& title) {
        auto* l = new QVBoxLayout();
        l->setContentsMargins(0, 0, 0, 0);
        l->addWidget(w);

        auto* gb = new QGroupBox(title);
        gb->setLayout(l);
        return gb;
    }

    QWidget* create_decoder_tab(QDialog* self) {
        this->dec_hexview = new QHexView();
        this->dec_hexview->setFrameStyle(QFrame::NoFrame);

        this->dec_textedit = new QPlainTextEdit();
        this->dec_textedit->setFrameStyle(QFrame::NoFrame);
        this->dec_textedit->setWordWrapMode(QTextOption::NoWrap);
        this->dec_textedit->setUndoRedoEnabled(false);
        this->dec_textedit->setReadOnly(true);

        auto* splitter = new QSplitter(Qt::Vertical);
        splitter->addWidget(this->group_widget(this->dec_hexview, "Input"));
        splitter->addWidget(this->group_widget(this->dec_textedit, "Decoded"));

        this->dec_buttonbox = new QDialogButtonBox(QDialogButtonBox::Ok |
                                                   QDialogButtonBox::Close);
        this->dec_buttonbox->button(QDialogButtonBox::Ok)->setText("Decode");

        QObject::connect(this->dec_buttonbox, &QDialogButtonBox::rejected, self,
                         &QDialog::reject);

        auto* hbox = new QHBoxLayout();
        hbox->addWidget(this->dec_cbxdetails);
        hbox->addStretch();
        hbox->addWidget(this->dec_buttonbox);

        auto* vbox = new QVBoxLayout(new QWidget());
        vbox->addWidget(splitter);
        vbox->addLayout(hbox);

        return vbox->parentWidget();
    }

    QWidget* create_encoder_tab(QDialog* self) {
        this->enc_textedit = new QPlainTextEdit();
        this->enc_textedit->setFrameStyle(QFrame::NoFrame);
        this->enc_textedit->setWordWrapMode(QTextOption::NoWrap);

        this->enc_hexview = new QHexView();
        this->enc_hexview->setFrameStyle(QFrame::NoFrame);
        this->enc_hexview->setReadOnly(true);

        auto* splitter = new QSplitter(Qt::Vertical);
        splitter->addWidget(this->group_widget(this->enc_textedit,
                                               "Instructions (one per line)"));
        splitter->addWidget(this->group_widget(this->enc_hexview, "Encoded"));

        this->enc_buttonbox = new QDialogButtonBox(QDialogButtonBox::Ok |
                                                   QDialogButtonBox::Close);
        this->enc_buttonbox->button(QDialogButtonBox::Ok)->setText("Encode");

        QObject::connect(this->enc_buttonbox, &QDialogButtonBox::rejected, self,
                         &QDialog::reject);

        this->enc_lblerror = new QLabel();
        QPalette p = this->enc_lblerror->palette();
        p.setColor(QPalette::WindowText, theme_provider::color(RD_THEME_FAIL));
        this->enc_lblerror->setPalette(p);

        auto* hbox = new QHBoxLayout();
        hbox->addWidget(this->enc_lblerror);
        hbox->addStretch();
        hbox->addWidget(this->enc_buttonbox);

        auto* vbox = new QVBoxLayout(new QWidget());
        vbox->addWidget(splitter);
        vbox->addLayout(hbox);

        return vbox->parentWidget();
    }
};

} // namespace ui
