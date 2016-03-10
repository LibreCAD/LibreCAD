#include "qg_activelayername.h"
#include "rs_settings.h"

QG_ActiveLayerName::QG_ActiveLayerName(QWidget *parent) :
    QWidget(parent)
{
    setupUi(this);

    init();
}

void QG_ActiveLayerName::activeLayerChanged(const QString& name)
{
    lActiveLayerName->setText(name);
}

void QG_ActiveLayerName::init() {
    lActiveLayerName->setText("");
}
