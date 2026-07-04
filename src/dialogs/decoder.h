#pragma once

#include "ui/decoderdialog.h"
#include <QDialog>

class DecoderDialog: public QDialog {
    Q_OBJECT

public:
    explicit DecoderDialog(QWidget* parent = nullptr);

private:
    void populate_processors() const;

private Q_SLOTS:
    void check_status();
    void do_decode();
    void do_encode();

private:
    ui::DecoderDialog m_ui;
};
