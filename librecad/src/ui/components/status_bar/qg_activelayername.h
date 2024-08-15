#ifndef QG_ACTIVELAYERNAME_H
#define QG_ACTIVELAYERNAME_H

#include "ui_qg_activelayername.h"


class QG_ActiveLayerName : public QWidget, public Ui::QG_ActiveLayerName
{
    Q_OBJECT
public:
    explicit QG_ActiveLayerName(QWidget *parent = 0);
    void activeLayerChanged(const QString& name);

};

#endif // QG_ACTIVELAYERNAME_H
