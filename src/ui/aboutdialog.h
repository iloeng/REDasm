#pragma once

#include <QDialog>
#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QTabWidget>
#include <QTextBrowser>
#include <QVBoxLayout>

namespace ui {

struct AboutDialog {
    QDialogButtonBox* buttonbox;
    QLabel *lbllogo, *lbltitle;
    QTextBrowser *txbconfig, *txbmodules, *txbloaders, *txbprocessors,
        *txbanalyzers, *txbcommands;
    QTabWidget* tabwidget;

    explicit AboutDialog(QDialog* self) {
        self->resize(800, 600);
        self->setAttribute(Qt::WA_DeleteOnClose);

        this->lbllogo = new QLabel();
        this->lbltitle = new QLabel();

        auto* hbox = new QHBoxLayout();
        hbox->addWidget(this->lbllogo);
        hbox->addWidget(this->lbltitle, 1);

        this->tabwidget = new QTabWidget(self);
        this->txbconfig = this->add_tab("Config");
        this->txbmodules = this->add_tab("Modules");
        this->txbloaders = this->add_tab("Loaders");
        this->txbprocessors = this->add_tab("Processors");
        this->txbanalyzers = this->add_tab("Analyzers");
        this->txbcommands = this->add_tab("Commands");

        this->buttonbox = new QDialogButtonBox(QDialogButtonBox::Close);
        QObject::connect(this->buttonbox, &QDialogButtonBox::rejected, self,
                         &QDialog::reject);

        auto* vbox = new QVBoxLayout(self);
        vbox->addLayout(hbox);
        vbox->addWidget(this->tabwidget);
        vbox->addWidget(this->buttonbox);
    }

private:
    [[nodiscard]] QTextBrowser* add_tab(const QString& title) const {
        auto* txb = new QTextBrowser();
        this->tabwidget->addTab(txb, title);
        return txb;
    }
};

} // namespace ui
