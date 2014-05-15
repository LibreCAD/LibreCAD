#include "qg_activelayername.h"

QG_ActiveLayerName::QG_ActiveLayerName(QWidget *parent) :
    QWidget(parent)
{
    setupUi(this);
}

void QG_ActiveLayerName::activeLayerChanged(const QString& name)
{
    lActiveLayerName->setText(name);
}
