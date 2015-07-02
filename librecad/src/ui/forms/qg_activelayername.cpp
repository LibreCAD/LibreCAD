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

    int fsize = 7;
#ifdef __APPLE__
    fsize = 9;
#endif

    RS_SETTINGS->beginGroup("/Appearance");
    fsize = RS_SETTINGS->readNumEntry("/StatusBarFontSize", fsize);
    RS_SETTINGS->endGroup();

    lActiveLayer->setFont(QFont("Helvetica", fsize));
    lActiveLayerName->setFont(QFont("Helvetica", fsize));
}
