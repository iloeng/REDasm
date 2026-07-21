#pragma once

#include "models/typedefsfilter.h"
#include "ui/typedefsdialog.h"
#include <redasm/redasm.h>

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
