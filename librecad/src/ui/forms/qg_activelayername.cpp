#include "qg_activelayername.h"
#include "rs_settings.h"
#include "lc_helpbrowser.h"

QG_ActiveLayerName::QG_ActiveLayerName(QWidget *parent) :
    QWidget(parent)
{
    setupUi(this);
	LC_HELP->associateTopic(this, "topic_guide_layers");

    lActiveLayerName->setText("");
}

void QG_ActiveLayerName::activeLayerChanged(const QString& name)
{
    lActiveLayerName->setText(name);
}
