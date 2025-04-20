#ifndef QG_ACTIVELAYERNAME_H
#define QG_ACTIVELAYERNAME_H

#include "lc_graphicviewaware.h"
#include "ui_qg_activelayername.h"

class QG_ActiveLayerName : public QWidget, public LC_GraphicViewAware, public Ui::QG_ActiveLayerName{
    Q_OBJECT
public:
    explicit QG_ActiveLayerName(QWidget *parent = nullptr);
    void activeLayerChanged(const QString& name);
    void setGraphicView(RS_GraphicView* gview) override;
};

#endif // QG_ACTIVELAYERNAME_H
