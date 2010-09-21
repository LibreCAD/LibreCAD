#ifndef QG_WIDGETPEN_H
#define QG_WIDGETPEN_H

#include <qvariant.h>


#include <Qt3Support/Q3ButtonGroup>
#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QGridLayout>
#include <QtGui/QHeaderView>
#include <QtGui/QLabel>
#include <QtGui/QSpacerItem>
#include <QtGui/QWidget>
#include "qg_colorbox.h"
#include "qg_linetypebox.h"
#include "qg_widthbox.h"
#include "rs_pen.h"

QT_BEGIN_NAMESPACE

class Ui_QG_WidgetPen
{
public:
    QGridLayout *gridLayout;
    Q3ButtonGroup *bgPen;
    QGridLayout *gridLayout1;
    QSpacerItem *spacer9;
    QLabel *lLineType;
    QSpacerItem *spacer8;
    QLabel *lWidth;
    QG_LineTypeBox *cbLineType;
    QG_ColorBox *cbColor;
    QLabel *lColor;
    QG_WidthBox *cbWidth;

    void setupUi(QWidget *QG_WidgetPen)
    {
        if (QG_WidgetPen->objectName().isEmpty())
            QG_WidgetPen->setObjectName(QString::fromUtf8("QG_WidgetPen"));
        QG_WidgetPen->resize(236, 121);
        gridLayout = new QGridLayout(QG_WidgetPen);
        gridLayout->setSpacing(0);
        gridLayout->setContentsMargins(0, 0, 0, 0);
        gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
        bgPen = new Q3ButtonGroup(QG_WidgetPen);
        bgPen->setObjectName(QString::fromUtf8("bgPen"));
        bgPen->setColumnLayout(0, Qt::Vertical);
        bgPen->layout()->setSpacing(6);
        bgPen->layout()->setContentsMargins(11, 11, 11, 11);
        gridLayout1 = new QGridLayout();
        QBoxLayout *boxlayout = qobject_cast<QBoxLayout *>(bgPen->layout());
        if (boxlayout)
            boxlayout->addLayout(gridLayout1);
        gridLayout1->setAlignment(Qt::AlignTop);
        gridLayout1->setObjectName(QString::fromUtf8("gridLayout1"));
        spacer9 = new QSpacerItem(21, 1, QSizePolicy::Minimum, QSizePolicy::Expanding);

        gridLayout1->addItem(spacer9, 3, 1, 1, 1);

        lLineType = new QLabel(bgPen);
        lLineType->setObjectName(QString::fromUtf8("lLineType"));
        QSizePolicy sizePolicy(static_cast<QSizePolicy::Policy>(3), static_cast<QSizePolicy::Policy>(5));
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(lLineType->sizePolicy().hasHeightForWidth());
        lLineType->setSizePolicy(sizePolicy);
        lLineType->setWordWrap(false);

        gridLayout1->addWidget(lLineType, 2, 0, 1, 1);

        spacer8 = new QSpacerItem(21, 1, QSizePolicy::Minimum, QSizePolicy::Expanding);

        gridLayout1->addItem(spacer8, 3, 0, 1, 1);

        lWidth = new QLabel(bgPen);
        lWidth->setObjectName(QString::fromUtf8("lWidth"));
        sizePolicy.setHeightForWidth(lWidth->sizePolicy().hasHeightForWidth());
        lWidth->setSizePolicy(sizePolicy);
        lWidth->setWordWrap(false);

        gridLayout1->addWidget(lWidth, 1, 0, 1, 1);

        cbLineType = new QG_LineTypeBox(bgPen);
        cbLineType->setObjectName(QString::fromUtf8("cbLineType"));

        gridLayout1->addWidget(cbLineType, 2, 1, 1, 1);

        cbColor = new QG_ColorBox(bgPen);
        cbColor->setObjectName(QString::fromUtf8("cbColor"));

        gridLayout1->addWidget(cbColor, 0, 1, 1, 1);

        lColor = new QLabel(bgPen);
        lColor->setObjectName(QString::fromUtf8("lColor"));
        sizePolicy.setHeightForWidth(lColor->sizePolicy().hasHeightForWidth());
        lColor->setSizePolicy(sizePolicy);
        lColor->setWordWrap(false);

        gridLayout1->addWidget(lColor, 0, 0, 1, 1);

        cbWidth = new QG_WidthBox(bgPen);
        cbWidth->setObjectName(QString::fromUtf8("cbWidth"));

        gridLayout1->addWidget(cbWidth, 1, 1, 1, 1);


        gridLayout->addWidget(bgPen, 0, 0, 1, 1);


        retranslateUi(QG_WidgetPen);

        QMetaObject::connectSlotsByName(QG_WidgetPen);
    } // setupUi

    void retranslateUi(QWidget *QG_WidgetPen)
    {
        QG_WidgetPen->setWindowTitle(QApplication::translate("QG_WidgetPen", "Pen", 0, QApplication::UnicodeUTF8));
        bgPen->setTitle(QApplication::translate("QG_WidgetPen", "Pen", 0, QApplication::UnicodeUTF8));
        lLineType->setText(QApplication::translate("QG_WidgetPen", "Line type:", 0, QApplication::UnicodeUTF8));
        lWidth->setText(QApplication::translate("QG_WidgetPen", "Width:", 0, QApplication::UnicodeUTF8));
        lColor->setText(QApplication::translate("QG_WidgetPen", "Color:", 0, QApplication::UnicodeUTF8));
    } // retranslateUi


protected:
    enum IconID
    {
        image0_ID,
        unknown_ID
    };
    static QPixmap qt_get_icon(IconID id)
    {
    static const char* const image0_data[] = { 
"0 0 0 1"};


    switch (id) {
        case image0_ID: return QPixmap((const char**)image0_data);
        default: return QPixmap();
    } // switch
    } // icon

};

namespace Ui {
    class QG_WidgetPen: public Ui_QG_WidgetPen {};
} // namespace Ui

QT_END_NAMESPACE

class QG_WidgetPen : public QWidget, public Ui::QG_WidgetPen
{
    Q_OBJECT

public:
    QG_WidgetPen(QWidget* parent = 0, const char* name = 0, Qt::WindowFlags fl = 0);
    ~QG_WidgetPen();

    virtual bool isColorUnchanged();
    virtual bool isLineTypeUnchanged();
    virtual bool isWidthUnchanged();

public slots:
    virtual void setPen( RS_Pen pen, bool showByLayer, bool showUnchanged, const QString & title );
    virtual RS_Pen getPen();

protected slots:
    virtual void languageChange();

};

#endif // QG_WIDGETPEN_H
