/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2010 R. van Twisk (librecad@rvt.dds.nl)
** Copyright (C) 2001-2003 RibbonSoft. All rights reserved.
**
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by 
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
** 
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
**
** This copyright notice MUST APPEAR in all copies of the script!  
**
**********************************************************************/
#ifndef QG_POLYLINEOPTIONS_H
#define QG_POLYLINEOPTIONS_H

#include <qvariant.h>


#include <Qt3Support/Q3ButtonGroup>
#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QComboBox>
#include <QtGui/QFrame>
#include <QtGui/QHBoxLayout>
#include <QtGui/QHeaderView>
#include <QtGui/QLabel>
#include <QtGui/QLineEdit>
#include <QtGui/QRadioButton>
#include <QtGui/QToolButton>
#include <QtGui/QWidget>
#include "rs_actiondrawpolyline.h"
#include "rs_line.h"
#include "rs_settings.h"

QT_BEGIN_NAMESPACE

class Ui_QG_PolylineOptions
{
public:
    QHBoxLayout *hboxLayout;
    QToolButton *bClose;
    QToolButton *bUndo;
    QComboBox *cbMode;
    QLabel *lRadius;
    QLineEdit *leRadius;
    QLabel *lAngle;
    QLineEdit *leAngle;
    Q3ButtonGroup *buttonGroup1;
    QRadioButton *rbNeg;
    QRadioButton *rbPos;
    QFrame *sep1;

    void setupUi(QWidget *QG_PolylineOptions)
    {
        if (QG_PolylineOptions->objectName().isEmpty())
            QG_PolylineOptions->setObjectName(QString::fromUtf8("QG_PolylineOptions"));
        QG_PolylineOptions->resize(650, 22);
        QSizePolicy sizePolicy(static_cast<QSizePolicy::Policy>(4), static_cast<QSizePolicy::Policy>(5));
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(QG_PolylineOptions->sizePolicy().hasHeightForWidth());
        QG_PolylineOptions->setSizePolicy(sizePolicy);
        QG_PolylineOptions->setMinimumSize(QSize(650, 22));
        QG_PolylineOptions->setMaximumSize(QSize(650, 22));
        hboxLayout = new QHBoxLayout(QG_PolylineOptions);
        hboxLayout->setSpacing(6);
        hboxLayout->setContentsMargins(1, 1, 1, 1);
        hboxLayout->setObjectName(QString::fromUtf8("hboxLayout"));
        bClose = new QToolButton(QG_PolylineOptions);
        bClose->setObjectName(QString::fromUtf8("bClose"));

        hboxLayout->addWidget(bClose);

        bUndo = new QToolButton(QG_PolylineOptions);
        bUndo->setObjectName(QString::fromUtf8("bUndo"));

        hboxLayout->addWidget(bUndo);

        cbMode = new QComboBox(QG_PolylineOptions);
        cbMode->setObjectName(QString::fromUtf8("cbMode"));
        QSizePolicy sizePolicy1(static_cast<QSizePolicy::Policy>(1), static_cast<QSizePolicy::Policy>(0));
        sizePolicy1.setHorizontalStretch(0);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(cbMode->sizePolicy().hasHeightForWidth());
        cbMode->setSizePolicy(sizePolicy1);

        hboxLayout->addWidget(cbMode);

        lRadius = new QLabel(QG_PolylineOptions);
        lRadius->setObjectName(QString::fromUtf8("lRadius"));
        lRadius->setAlignment(Qt::AlignVCenter);
        lRadius->setWordWrap(false);

        hboxLayout->addWidget(lRadius);

        leRadius = new QLineEdit(QG_PolylineOptions);
        leRadius->setObjectName(QString::fromUtf8("leRadius"));

        hboxLayout->addWidget(leRadius);

        lAngle = new QLabel(QG_PolylineOptions);
        lAngle->setObjectName(QString::fromUtf8("lAngle"));
        lAngle->setAlignment(Qt::AlignVCenter);
        lAngle->setWordWrap(false);

        hboxLayout->addWidget(lAngle);

        leAngle = new QLineEdit(QG_PolylineOptions);
        leAngle->setObjectName(QString::fromUtf8("leAngle"));

        hboxLayout->addWidget(leAngle);

        buttonGroup1 = new Q3ButtonGroup(QG_PolylineOptions);
        buttonGroup1->setObjectName(QString::fromUtf8("buttonGroup1"));
        buttonGroup1->setLineWidth(0);
        buttonGroup1->setFlat(true);
        rbNeg = new QRadioButton(buttonGroup1);
        rbNeg->setObjectName(QString::fromUtf8("rbNeg"));
        rbNeg->setGeometry(QRect(38, 2, 36, 19));
        rbNeg->setIcon(qt_get_icon(image0_ID));
        rbPos = new QRadioButton(buttonGroup1);
        rbPos->setObjectName(QString::fromUtf8("rbPos"));
        rbPos->setGeometry(QRect(2, 2, 36, 19));
        rbPos->setIcon(qt_get_icon(image1_ID));
        rbPos->setChecked(true);

        hboxLayout->addWidget(buttonGroup1);

        sep1 = new QFrame(QG_PolylineOptions);
        sep1->setObjectName(QString::fromUtf8("sep1"));
        QSizePolicy sizePolicy2(static_cast<QSizePolicy::Policy>(0), static_cast<QSizePolicy::Policy>(1));
        sizePolicy2.setHorizontalStretch(0);
        sizePolicy2.setVerticalStretch(0);
        sizePolicy2.setHeightForWidth(sep1->sizePolicy().hasHeightForWidth());
        sep1->setSizePolicy(sizePolicy2);
        sep1->setFrameShape(QFrame::VLine);
        sep1->setFrameShadow(QFrame::Sunken);

        hboxLayout->addWidget(sep1);


        retranslateUi(QG_PolylineOptions);
        QObject::connect(bClose, SIGNAL(clicked()), QG_PolylineOptions, SLOT(close()));
        QObject::connect(bUndo, SIGNAL(clicked()), QG_PolylineOptions, SLOT(undo()));
        QObject::connect(leRadius, SIGNAL(textChanged(QString)), QG_PolylineOptions, SLOT(updateRadius(QString)));
        QObject::connect(leAngle, SIGNAL(textChanged(QString)), QG_PolylineOptions, SLOT(updateAngle(QString)));
        QObject::connect(cbMode, SIGNAL(activated(int)), QG_PolylineOptions, SLOT(updateMode(int)));
        QObject::connect(rbNeg, SIGNAL(toggled(bool)), QG_PolylineOptions, SLOT(updateDirection(bool)));
        QObject::connect(rbPos, SIGNAL(toggled(bool)), QG_PolylineOptions, SLOT(updateDirection(bool)));

        QMetaObject::connectSlotsByName(QG_PolylineOptions);
    } // setupUi

    void retranslateUi(QWidget *QG_PolylineOptions)
    {
        QG_PolylineOptions->setWindowTitle(QApplication::translate("QG_PolylineOptions", "Polyline Options", 0, QApplication::UnicodeUTF8));
        bClose->setText(QApplication::translate("QG_PolylineOptions", "Close", 0, QApplication::UnicodeUTF8));
        bUndo->setText(QApplication::translate("QG_PolylineOptions", "Undo", 0, QApplication::UnicodeUTF8));
        cbMode->clear();
        cbMode->insertItems(0, QStringList()
         << QApplication::translate("QG_PolylineOptions", "Line", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("QG_PolylineOptions", "Tangential", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("QG_PolylineOptions", "Tan Radius", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("QG_PolylineOptions", "Angle", 0, QApplication::UnicodeUTF8)
        );
        lRadius->setText(QApplication::translate("QG_PolylineOptions", "Radius:", 0, QApplication::UnicodeUTF8));
        lAngle->setText(QApplication::translate("QG_PolylineOptions", "Angle:", 0, QApplication::UnicodeUTF8));
        buttonGroup1->setTitle(QString());
        rbNeg->setText(QString());
#ifndef QT_NO_TOOLTIP
        rbNeg->setProperty("toolTip", QVariant(QApplication::translate("QG_PolylineOptions", "Clockwise", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
        rbPos->setText(QString());
#ifndef QT_NO_TOOLTIP
        rbPos->setProperty("toolTip", QVariant(QApplication::translate("QG_PolylineOptions", "Counter Clockwise", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
    } // retranslateUi


protected:
    enum IconID
    {
        image0_ID,
        image1_ID,
        unknown_ID
    };
    static QPixmap qt_get_icon(IconID id)
    {
    static const unsigned char image0_data[] = { 
    0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a, 0x00, 0x00, 0x00, 0x0d,
    0x49, 0x48, 0x44, 0x52, 0x00, 0x00, 0x00, 0x0e, 0x00, 0x00, 0x00, 0x0f,
    0x08, 0x06, 0x00, 0x00, 0x00, 0xd4, 0x14, 0xfe, 0x74, 0x00, 0x00, 0x00,
    0x73, 0x49, 0x44, 0x41, 0x54, 0x28, 0x91, 0xbd, 0x93, 0x51, 0x0a, 0xc0,
    0x20, 0x0c, 0x43, 0x53, 0xf1, 0x52, 0xe6, 0xfe, 0x64, 0xc7, 0x72, 0x1f,
    0xc3, 0x21, 0xa5, 0x62, 0xdd, 0xc7, 0xf2, 0x59, 0xfa, 0xd2, 0x16, 0xa3,
    0x49, 0xc2, 0xa9, 0x1a, 0xd9, 0xab, 0x2f, 0x92, 0xec, 0x51, 0xb3, 0x24,
    0x1b, 0x10, 0x00, 0x54, 0x0f, 0x8d, 0x86, 0xc8, 0x70, 0x76, 0x2c, 0x19,
    0x08, 0x00, 0xfc, 0x1a, 0x25, 0x03, 0xb5, 0x60, 0x7d, 0x03, 0x70, 0x0c,
    0x01, 0xee, 0xc6, 0x48, 0xd7, 0xc2, 0xb4, 0x44, 0xc5, 0x8c, 0xfe, 0x05,
    0x49, 0xf6, 0xef, 0x13, 0x25, 0xd9, 0x2a, 0x2d, 0xab, 0x69, 0x92, 0xac,
    0x00, 0x4f, 0x9c, 0x32, 0xf0, 0xfc, 0xde, 0xef, 0xaa, 0x3b, 0xd8, 0x87,
    0xc4, 0xfc, 0xef, 0xd8, 0x85, 0x7c, 0xe8, 0x06, 0x72, 0xd2, 0x42, 0x74,
    0xc7, 0x65, 0x63, 0xdb, 0x00, 0x00, 0x00, 0x00, 0x49, 0x45, 0x4e, 0x44,
    0xae, 0x42, 0x60, 0x82
};

    static const unsigned char image1_data[] = { 
    0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a, 0x00, 0x00, 0x00, 0x0d,
    0x49, 0x48, 0x44, 0x52, 0x00, 0x00, 0x00, 0x0e, 0x00, 0x00, 0x00, 0x0f,
    0x08, 0x06, 0x00, 0x00, 0x00, 0xd4, 0x14, 0xfe, 0x74, 0x00, 0x00, 0x00,
    0x6b, 0x49, 0x44, 0x41, 0x54, 0x28, 0x91, 0xbd, 0x92, 0x4b, 0x0e, 0x80,
    0x20, 0x0c, 0x44, 0xa7, 0x8d, 0x97, 0xa2, 0xf7, 0xcf, 0x78, 0x2c, 0x5c,
    0x61, 0xc8, 0x58, 0x42, 0x75, 0xe1, 0xdb, 0x11, 0xfa, 0xd2, 0xaf, 0x91,
    0xc4, 0x4c, 0x44, 0x74, 0x24, 0x90, 0xb4, 0xf9, 0x7d, 0xa8, 0xa4, 0x01,
    0xab, 0x3f, 0xaf, 0x48, 0x23, 0xe3, 0x5c, 0x8d, 0x57, 0xa4, 0x4c, 0x36,
    0x00, 0x25, 0x49, 0xcb, 0xf6, 0x7d, 0x58, 0xce, 0x27, 0x91, 0xa4, 0xfd,
    0x9b, 0x11, 0x90, 0x3d, 0x66, 0xb4, 0xc5, 0x41, 0xb8, 0xee, 0x47, 0x39,
    0x17, 0x13, 0x77, 0xe0, 0xb9, 0xdc, 0x8a, 0x7c, 0xf7, 0xb8, 0x93, 0xd5,
    0xb4, 0xb7, 0x47, 0x3e, 0x7a, 0x7e, 0x88, 0x15, 0x5a, 0x44, 0xbf, 0x00,
    0x11, 0xfe, 0x44, 0x14, 0x24, 0x29, 0x5e, 0x9a, 0x00, 0x00, 0x00, 0x00,
    0x49, 0x45, 0x4e, 0x44, 0xae, 0x42, 0x60, 0x82
};

    switch (id) {
        case image0_ID:  { QImage img; img.loadFromData(image0_data, sizeof(image0_data), "PNG"); return QPixmap::fromImage(img); }
        case image1_ID:  { QImage img; img.loadFromData(image1_data, sizeof(image1_data), "PNG"); return QPixmap::fromImage(img); }
        default: return QPixmap();
    } // switch
    } // icon

};

namespace Ui {
    class QG_PolylineOptions: public Ui_QG_PolylineOptions {};
} // namespace Ui

QT_END_NAMESPACE

class QG_PolylineOptions : public QWidget, public Ui::QG_PolylineOptions
{
    Q_OBJECT

public:
    QG_PolylineOptions(QWidget* parent = 0, const char* name = 0, Qt::WindowFlags fl = 0);
    ~QG_PolylineOptions();

public slots:
    virtual void setAction( RS_ActionInterface * a, bool update );
    virtual void close();
    virtual void undo();
    virtual void updateRadius( const QString & s );
    virtual void updateAngle( const QString & s );
    virtual void updateDirection( bool );
    virtual void updateMode( int m );

protected:
    RS_ActionDrawPolyline* action;

protected slots:
    virtual void languageChange();

private:
    void destroy();

};

#endif // QG_POLYLINEOPTIONS_H
