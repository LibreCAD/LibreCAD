#include "qg_truecolordlg.h"
#include <QHBoxLayout>

QG_TrueColorDlg::QG_TrueColorDlg(const QColor &initial, QWidget *parent, bool noButton)
    : QColorDialog(initial, parent)
    , m_index(-1)
{
    if (!noButton)
    {
        QWidget *w = new QWidget(this);
        QHBoxLayout *buttonLayout = new QHBoxLayout;

        QPushButton *buttonLayer = new QPushButton(tr("By &Layer"));
        QPushButton *buttonBlock = new QPushButton(tr("By Bloc&k"));
        buttonLayer->setMaximumWidth(80);
        buttonBlock->setMaximumWidth(80);

        connect(buttonLayer, &QPushButton::pressed, this, &QG_TrueColorDlg::setByLayer);
        connect(buttonBlock, &QPushButton::pressed, this, &QG_TrueColorDlg::setByBlock);

        buttonLayout->addSpacing(48);
        buttonLayout->addWidget(buttonLayer);
        buttonLayout->addWidget(buttonBlock);
        buttonLayout->setAlignment(Qt::AlignLeft);
        w->setLayout(buttonLayout);
        this->layout()->addWidget(w);
    }
}
