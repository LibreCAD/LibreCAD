#ifndef QG_COORDINATEWIDGET_H
#define QG_COORDINATEWIDGET_H

#include <qvariant.h>


#include <Qt3Support/Q3MimeSourceFactory>
#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QFrame>
#include <QtGui/QHBoxLayout>
#include <QtGui/QHeaderView>
#include <QtGui/QLabel>
#include <QtGui/QVBoxLayout>
#include <QtGui/QWidget>
#include "rs_graphic.h"
#include "rs_math.h"
#include "rs_vector.h"

QT_BEGIN_NAMESPACE

class Ui_QG_CoordinateWidget
{
public:
    QHBoxLayout *hboxLayout;
    QVBoxLayout *vboxLayout;
    QLabel *lCoord1;
    QLabel *lCoord1b;
    QFrame *line1;
    QVBoxLayout *vboxLayout1;
    QLabel *lCoord2;
    QLabel *lCoord2b;

    void setupUi(QWidget *QG_CoordinateWidget)
    {
        if (QG_CoordinateWidget->objectName().isEmpty())
            QG_CoordinateWidget->setObjectName(QString::fromUtf8("QG_CoordinateWidget"));
        QG_CoordinateWidget->resize(380, 27);
        QSizePolicy sizePolicy(static_cast<QSizePolicy::Policy>(1), static_cast<QSizePolicy::Policy>(1));
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(QG_CoordinateWidget->sizePolicy().hasHeightForWidth());
        QG_CoordinateWidget->setSizePolicy(sizePolicy);
        QG_CoordinateWidget->setMinimumSize(QSize(380, 27));
        QG_CoordinateWidget->setMaximumSize(QSize(500, 50));
        hboxLayout = new QHBoxLayout(QG_CoordinateWidget);
        hboxLayout->setSpacing(0);
        hboxLayout->setContentsMargins(0, 0, 0, 0);
        hboxLayout->setObjectName(QString::fromUtf8("hboxLayout"));
        vboxLayout = new QVBoxLayout();
        vboxLayout->setSpacing(0);
        vboxLayout->setContentsMargins(0, 0, 0, 0);
        vboxLayout->setObjectName(QString::fromUtf8("vboxLayout"));
        lCoord1 = new QLabel(QG_CoordinateWidget);
        lCoord1->setObjectName(QString::fromUtf8("lCoord1"));
        QFont font;
        font.setFamily(QString::fromUtf8("Helvetica"));
        font.setPointSize(7);
        lCoord1->setFont(font);
        lCoord1->setFrameShape(QFrame::NoFrame);
        lCoord1->setFrameShadow(QFrame::Plain);
        lCoord1->setWordWrap(false);

        vboxLayout->addWidget(lCoord1);

        lCoord1b = new QLabel(QG_CoordinateWidget);
        lCoord1b->setObjectName(QString::fromUtf8("lCoord1b"));
        lCoord1b->setFont(font);
        lCoord1b->setFrameShape(QFrame::NoFrame);
        lCoord1b->setFrameShadow(QFrame::Plain);
        lCoord1b->setWordWrap(false);

        vboxLayout->addWidget(lCoord1b);


        hboxLayout->addLayout(vboxLayout);

        line1 = new QFrame(QG_CoordinateWidget);
        line1->setObjectName(QString::fromUtf8("line1"));
        line1->setFrameShape(QFrame::VLine);
        line1->setFrameShadow(QFrame::Sunken);

        hboxLayout->addWidget(line1);

        vboxLayout1 = new QVBoxLayout();
        vboxLayout1->setSpacing(0);
        vboxLayout1->setContentsMargins(0, 0, 0, 0);
        vboxLayout1->setObjectName(QString::fromUtf8("vboxLayout1"));
        lCoord2 = new QLabel(QG_CoordinateWidget);
        lCoord2->setObjectName(QString::fromUtf8("lCoord2"));
        lCoord2->setFont(font);
        lCoord2->setWordWrap(false);

        vboxLayout1->addWidget(lCoord2);

        lCoord2b = new QLabel(QG_CoordinateWidget);
        lCoord2b->setObjectName(QString::fromUtf8("lCoord2b"));
        lCoord2b->setFont(font);
        lCoord2b->setWordWrap(false);

        vboxLayout1->addWidget(lCoord2b);


        hboxLayout->addLayout(vboxLayout1);


        retranslateUi(QG_CoordinateWidget);

        QMetaObject::connectSlotsByName(QG_CoordinateWidget);
    } // setupUi

    void retranslateUi(QWidget *QG_CoordinateWidget)
    {
        QG_CoordinateWidget->setWindowTitle(QApplication::translate("QG_CoordinateWidget", "Coordinates", 0, QApplication::UnicodeUTF8));
        lCoord1->setText(QApplication::translate("QG_CoordinateWidget", "Coordinates", 0, QApplication::UnicodeUTF8));
        lCoord1b->setText(QApplication::translate("QG_CoordinateWidget", "Coordinates", 0, QApplication::UnicodeUTF8));
        lCoord2->setText(QApplication::translate("QG_CoordinateWidget", "Coordinates", 0, QApplication::UnicodeUTF8));
        lCoord2b->setText(QApplication::translate("QG_CoordinateWidget", "Coordinates", 0, QApplication::UnicodeUTF8));
    } // retranslateUi

};

namespace Ui {
    class QG_CoordinateWidget: public Ui_QG_CoordinateWidget {};
} // namespace Ui

QT_END_NAMESPACE

class QG_CoordinateWidget : public QWidget, public Ui::QG_CoordinateWidget
{
    Q_OBJECT

public:
    QG_CoordinateWidget(QWidget* parent = 0, const char* name = 0, Qt::WindowFlags fl = 0);
    ~QG_CoordinateWidget();

public slots:
    virtual void init();
    virtual void setGraphic( RS_Graphic * graphic );
    virtual void setCoordinates( const RS_Vector & abs, const RS_Vector & rel, bool updateFormat );
    virtual void setCoordinates( double x, double y, double rx, double ry, bool updateFormat );

protected slots:
    virtual void languageChange();

private:
    RS_Graphic* graphic;
    int prec;
    RS2::LinearFormat format;
    int aprec;
    RS2::AngleFormat aformat;

};

#endif // QG_COORDINATEWIDGET_H
