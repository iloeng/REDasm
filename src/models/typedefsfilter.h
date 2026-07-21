#pragma once

#include "typedefs.h"
#include <QSortFilterProxyModel>
#include <redasm/redasm.h>

class TypedefsFilterModel: public QSortFilterProxyModel {
    Q_OBJECT

public:
    explicit TypedefsFilterModel(RDContext* ctx, QObject* parent = nullptr);
    [[nodiscard]] const RDTypeDef* type_def(const QModelIndex& index) const;

    [[nodiscard]] const TypedefsModel* typedefs_model() const {
        return qobject_cast<const TypedefsModel*>(this->sourceModel());
    }

protected:
    [[nodiscard]] bool filterAcceptsRow(int source_row,
                                        const QModelIndex&) const override;
};
