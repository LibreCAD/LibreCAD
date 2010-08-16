/****************************************************************************
**
** This file is part of the CADuntu project, a 2D CAD program
**
** Copyright (C) 2010 R. van Twisk (caduntu@rvt.dds.nl)
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
#ifndef QG_DIMLINEAROPTIONS_H
#define QG_DIMLINEAROPTIONS_H

#include <qvariant.h>


#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QFrame>
#include <QtGui/QHBoxLayout>
#include <QtGui/QHeaderView>
#include <QtGui/QLabel>
#include <QtGui/QLineEdit>
#include <QtGui/QToolButton>
#include <QtGui/QWidget>
#include "rs_actiondimlinear.h"
#include "rs_dimlinear.h"
#include "rs_settings.h"

QT_BEGIN_NAMESPACE

class Ui_QG_DimLinearOptions
{
public:
    QHBoxLayout *hboxLayout;
    QLabel *lAngle;
    QLineEdit *leAngle;
    QToolButton *bHor;
    QToolButton *bVer;
    QFrame *sep1;

    void setupUi(QWidget *QG_DimLinearOptions)
    {
        if (QG_DimLinearOptions->objectName().isEmpty())
            QG_DimLinearOptions->setObjectName(QString::fromUtf8("QG_DimLinearOptions"));
        QG_DimLinearOptions->resize(200, 22);
        QSizePolicy sizePolicy(static_cast<QSizePolicy::Policy>(5), static_cast<QSizePolicy::Policy>(0));
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(QG_DimLinearOptions->sizePolicy().hasHeightForWidth());
        QG_DimLinearOptions->setSizePolicy(sizePolicy);
        QG_DimLinearOptions->setMinimumSize(QSize(180, 22));
        QG_DimLinearOptions->setMaximumSize(QSize(200, 22));
        hboxLayout = new QHBoxLayout(QG_DimLinearOptions);
        hboxLayout->setSpacing(6);
        hboxLayout->setContentsMargins(1, 1, 1, 1);
        hboxLayout->setObjectName(QString::fromUtf8("hboxLayout"));
        lAngle = new QLabel(QG_DimLinearOptions);
        lAngle->setObjectName(QString::fromUtf8("lAngle"));
        QSizePolicy sizePolicy1(static_cast<QSizePolicy::Policy>(0), static_cast<QSizePolicy::Policy>(0));
        sizePolicy1.setHorizontalStretch(0);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(lAngle->sizePolicy().hasHeightForWidth());
        lAngle->setSizePolicy(sizePolicy1);
        lAngle->setMinimumSize(QSize(0, 19));
        lAngle->setFrameShape(QFrame::NoFrame);
        lAngle->setFrameShadow(QFrame::Plain);
        lAngle->setWordWrap(false);

        hboxLayout->addWidget(lAngle);

        leAngle = new QLineEdit(QG_DimLinearOptions);
        leAngle->setObjectName(QString::fromUtf8("leAngle"));
        sizePolicy.setHeightForWidth(leAngle->sizePolicy().hasHeightForWidth());
        leAngle->setSizePolicy(sizePolicy);
        leAngle->setMinimumSize(QSize(0, 19));

        hboxLayout->addWidget(leAngle);

        bHor = new QToolButton(QG_DimLinearOptions);
        bHor->setObjectName(QString::fromUtf8("bHor"));
        sizePolicy1.setHeightForWidth(bHor->sizePolicy().hasHeightForWidth());
        bHor->setSizePolicy(sizePolicy1);
        bHor->setMinimumSize(QSize(0, 19));
        bHor->setIcon(qt_get_icon(image0_ID));

        hboxLayout->addWidget(bHor);

        bVer = new QToolButton(QG_DimLinearOptions);
        bVer->setObjectName(QString::fromUtf8("bVer"));
        sizePolicy1.setHeightForWidth(bVer->sizePolicy().hasHeightForWidth());
        bVer->setSizePolicy(sizePolicy1);
        bVer->setMinimumSize(QSize(0, 19));
        bVer->setIcon(qt_get_icon(image1_ID));

        hboxLayout->addWidget(bVer);

        sep1 = new QFrame(QG_DimLinearOptions);
        sep1->setObjectName(QString::fromUtf8("sep1"));
        QSizePolicy sizePolicy2(static_cast<QSizePolicy::Policy>(0), static_cast<QSizePolicy::Policy>(1));
        sizePolicy2.setHorizontalStretch(0);
        sizePolicy2.setVerticalStretch(0);
        sizePolicy2.setHeightForWidth(sep1->sizePolicy().hasHeightForWidth());
        sep1->setSizePolicy(sizePolicy2);
        sep1->setMinimumSize(QSize(0, 19));
        sep1->setFrameShape(QFrame::VLine);
        sep1->setFrameShadow(QFrame::Sunken);

        hboxLayout->addWidget(sep1);


        retranslateUi(QG_DimLinearOptions);
        QObject::connect(leAngle, SIGNAL(textChanged(QString)), QG_DimLinearOptions, SLOT(updateAngle(QString)));
        QObject::connect(bHor, SIGNAL(clicked()), QG_DimLinearOptions, SLOT(setHor()));
        QObject::connect(bVer, SIGNAL(clicked()), QG_DimLinearOptions, SLOT(setVer()));

        QMetaObject::connectSlotsByName(QG_DimLinearOptions);
    } // setupUi

    void retranslateUi(QWidget *QG_DimLinearOptions)
    {
        QG_DimLinearOptions->setWindowTitle(QApplication::translate("QG_DimLinearOptions", "Linear Dimension Options", 0, QApplication::UnicodeUTF8));
        lAngle->setText(QApplication::translate("QG_DimLinearOptions", "Angle:", 0, QApplication::UnicodeUTF8));
        leAngle->setText(QString());
        bHor->setText(QString());
        bVer->setText(QString());
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
    static const char* const image0_data[] = { 
"15 10 2 1",
"# c None",
". c #000000",
".#############.",
".##.#######.##.",
".#.#########.#.",
"...............",
".#.#########.#.",
".##.#######.##.",
".#############.",
".#############.",
".#############.",
".#############."};


    static const char* const image1_data[] = { 
"10 15 2 1",
"# c None",
". c #000000",
"..........",
"###.######",
"##...#####",
"#.#.#.####",
"###.######",
"###.######",
"###.######",
"###.######",
"###.######",
"###.######",
"###.######",
"#.#.#.####",
"##...#####",
"###.######",
".........."};


    switch (id) {
        case image0_ID: return QPixmap((const char**)image0_data);
        case image1_ID: return QPixmap((const char**)image1_data);
        default: return QPixmap();
    } // switch
    } // icon

};

namespace Ui {
    class QG_DimLinearOptions: public Ui_QG_DimLinearOptions {};
} // namespace Ui

QT_END_NAMESPACE

class QG_DimLinearOptions : public QWidget, public Ui::QG_DimLinearOptions
{
    Q_OBJECT

public:
    QG_DimLinearOptions(QWidget* parent = 0, const char* name = 0, Qt::WindowFlags fl = 0);
    ~QG_DimLinearOptions();

public slots:
    virtual void setAction( RS_ActionInterface * a, bool update );
    virtual void updateAngle( const QString & a );
    virtual void setHor();
    virtual void setVer();

protected:
    RS_ActionDimLinear* action;

protected slots:
    virtual void languageChange();

private:
    void destroy();

};

#endif // QG_DIMLINEAROPTIONS_H
