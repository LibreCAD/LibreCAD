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
#ifndef QG_CADTOOLBARCIRCLES_H
#define QG_CADTOOLBARCIRCLES_H

#include <qvariant.h>

class QG_CadToolBar;

#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QHeaderView>
#include <QtGui/QToolButton>
#include <QtGui/QWidget>
#include "qg_actionhandler.h"

QT_BEGIN_NAMESPACE

class Ui_QG_CadToolBarCircles
{
public:
    QToolButton *bCircle2P;
    QToolButton *bCircleCR;
    QToolButton *bCircle;
    QToolButton *bCircle3P;
    QToolButton *bBack;
    QToolButton *bCircleParallel;

    void setupUi(QWidget *QG_CadToolBarCircles)
    {
        if (QG_CadToolBarCircles->objectName().isEmpty())
            QG_CadToolBarCircles->setObjectName(QString::fromUtf8("QG_CadToolBarCircles"));
        QG_CadToolBarCircles->resize(56, 336);
        QSizePolicy sizePolicy(static_cast<QSizePolicy::Policy>(0), static_cast<QSizePolicy::Policy>(1));
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(QG_CadToolBarCircles->sizePolicy().hasHeightForWidth());
        QG_CadToolBarCircles->setSizePolicy(sizePolicy);
        QG_CadToolBarCircles->setMinimumSize(QSize(56, 336));
        bCircle2P = new QToolButton(QG_CadToolBarCircles);
        bCircle2P->setObjectName(QString::fromUtf8("bCircle2P"));
        bCircle2P->setGeometry(QRect(0, 48, 28, 28));
        bCircle2P->setIcon(qt_get_icon(image0_ID));
        bCircleCR = new QToolButton(QG_CadToolBarCircles);
        bCircleCR->setObjectName(QString::fromUtf8("bCircleCR"));
        bCircleCR->setGeometry(QRect(28, 20, 28, 28));
        bCircleCR->setIcon(qt_get_icon(image1_ID));
        bCircle = new QToolButton(QG_CadToolBarCircles);
        bCircle->setObjectName(QString::fromUtf8("bCircle"));
        bCircle->setGeometry(QRect(0, 20, 28, 28));
        bCircle->setIcon(qt_get_icon(image2_ID));
        bCircle3P = new QToolButton(QG_CadToolBarCircles);
        bCircle3P->setObjectName(QString::fromUtf8("bCircle3P"));
        bCircle3P->setGeometry(QRect(28, 48, 28, 28));
        bCircle3P->setIcon(qt_get_icon(image3_ID));
        bBack = new QToolButton(QG_CadToolBarCircles);
        bBack->setObjectName(QString::fromUtf8("bBack"));
        bBack->setGeometry(QRect(0, 0, 56, 20));
        bBack->setIcon(qt_get_icon(image4_ID));
        bCircleParallel = new QToolButton(QG_CadToolBarCircles);
        bCircleParallel->setObjectName(QString::fromUtf8("bCircleParallel"));
        bCircleParallel->setGeometry(QRect(0, 76, 28, 28));
        bCircleParallel->setIcon(qt_get_icon(image5_ID));
        QWidget::setTabOrder(bBack, bCircle);
        QWidget::setTabOrder(bCircle, bCircleCR);
        QWidget::setTabOrder(bCircleCR, bCircle2P);
        QWidget::setTabOrder(bCircle2P, bCircle3P);
        QWidget::setTabOrder(bCircle3P, bCircleParallel);

        retranslateUi(QG_CadToolBarCircles);
        QObject::connect(bCircle, SIGNAL(clicked()), QG_CadToolBarCircles, SLOT(drawCircle()));
        QObject::connect(bCircleCR, SIGNAL(clicked()), QG_CadToolBarCircles, SLOT(drawCircleCR()));
        QObject::connect(bCircle2P, SIGNAL(clicked()), QG_CadToolBarCircles, SLOT(drawCircle2P()));
        QObject::connect(bCircle3P, SIGNAL(clicked()), QG_CadToolBarCircles, SLOT(drawCircle3P()));
        QObject::connect(bCircleParallel, SIGNAL(clicked()), QG_CadToolBarCircles, SLOT(drawCircleParallel()));
        QObject::connect(bBack, SIGNAL(clicked()), QG_CadToolBarCircles, SLOT(back()));

        QMetaObject::connectSlotsByName(QG_CadToolBarCircles);
    } // setupUi

    void retranslateUi(QWidget *QG_CadToolBarCircles)
    {
        QG_CadToolBarCircles->setWindowTitle(QApplication::translate("QG_CadToolBarCircles", "Circles", 0, QApplication::UnicodeUTF8));
        bCircle2P->setText(QString());
#ifndef QT_NO_TOOLTIP
        bCircle2P->setProperty("toolTip", QVariant(QApplication::translate("QG_CadToolBarCircles", "Circle with two opposite points", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
        bCircleCR->setText(QString());
#ifndef QT_NO_TOOLTIP
        bCircleCR->setProperty("toolTip", QVariant(QApplication::translate("QG_CadToolBarCircles", "Circle with center and radius", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
        bCircle->setText(QString());
#ifndef QT_NO_TOOLTIP
        bCircle->setProperty("toolTip", QVariant(QApplication::translate("QG_CadToolBarCircles", "Circle with center and point", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
        bCircle3P->setText(QString());
#ifndef QT_NO_TOOLTIP
        bCircle3P->setProperty("toolTip", QVariant(QApplication::translate("QG_CadToolBarCircles", "Circle with three points", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
        bBack->setText(QString());
#ifndef QT_NO_TOOLTIP
        bBack->setProperty("toolTip", QVariant(QApplication::translate("QG_CadToolBarCircles", "Back to main menu", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
        bCircleParallel->setText(QString());
#ifndef QT_NO_TOOLTIP
        bCircleParallel->setProperty("toolTip", QVariant(QApplication::translate("QG_CadToolBarCircles", "Concentric", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
    } // retranslateUi


protected:
    enum IconID
    {
        image0_ID,
        image1_ID,
        image2_ID,
        image3_ID,
        image4_ID,
        image5_ID,
        unknown_ID
    };
    static QPixmap qt_get_icon(IconID id)
    {
    static const char* const image0_data[] = { 
"18 18 3 1",
". c None",
"# c #000000",
"a c #ff0000",
"......######......",
"....##......##....",
"...#..........aa..",
"..#...........aa..",
".#..............#.",
".#..............#.",
"#................#",
"#................#",
"#................#",
"#................#",
"#................#",
"#................#",
".#..............#.",
".#..............#.",
"..aa...........#..",
"..aa..........#...",
"....##......##....",
"......######......"};


    static const char* const image1_data[] = { 
"18 18 3 1",
". c None",
"# c #000000",
"a c #ff0000",
"......######......",
"....##......##....",
"...#..........#...",
"..#...........a#..",
".#...........a..#.",
".#..........a...#.",
"#..........a.....#",
"#.........a......#",
"#.......aa.......#",
"#.......aa.......#",
"#................#",
"#................#",
".#..............#.",
".#..............#.",
"..#............#..",
"...#..........#...",
"....##......##....",
"......######......"};


    static const char* const image2_data[] = { 
"18 18 3 1",
". c None",
"# c #000000",
"a c #ff0000",
"......######......",
"....##......##....",
"...#..........aa..",
"..#...........aa..",
".#..............#.",
".#..............#.",
"#................#",
"#................#",
"#.......aa.......#",
"#.......aa.......#",
"#................#",
"#................#",
".#..............#.",
".#..............#.",
"..#............#..",
"...#..........#...",
"....##......##....",
"......######......"};


    static const char* const image3_data[] = { 
"18 18 3 1",
". c None",
"# c #000000",
"a c #ff0000",
"......######......",
"...aa#......##....",
"...aa.........aa..",
"..#...........aa..",
".#..............#.",
".#..............#.",
"#................#",
"#................#",
"#................#",
"#................#",
"#................#",
"#................#",
".aa.............#.",
".aa.............#.",
"..#............#..",
"...#..........#...",
"....##......##....",
"......######......"};


    static const char* const image4_data[] = { 
"16 11 3 1",
". c None",
"a c #000000",
"# c #ffffff",
"....#a..........",
"...#aa..........",
"..#aaa######....",
".#aaaaaaaaaaa...",
"#aaaaaaaaaaaa...",
"aaaaaaaaaaaaa...",
".aaaaaaaaaaaa...",
"..aaaaaaaaaaa...",
"...aaa..........",
"....aa..........",
".....a.........."};


    static const char* const image5_data[] = { 
"16 16 3 1",
". c None",
"# c #000000",
"a c #ff0000",
".....######.....",
"...##......##...",
"..#..........#..",
".#....aaaa....#.",
".#..aa....aa..#.",
"#...a......a...#",
"#..a...##...a..#",
"#..a..#..#..a..#",
"#..a..#..#..a..#",
"#..a...##...a..#",
"#...a......a...#",
".#..aa....aa..#.",
".#....aaaa....#.",
"..#..........#..",
"...##......##...",
".....######....."};


    switch (id) {
        case image0_ID: return QPixmap((const char**)image0_data);
        case image1_ID: return QPixmap((const char**)image1_data);
        case image2_ID: return QPixmap((const char**)image2_data);
        case image3_ID: return QPixmap((const char**)image3_data);
        case image4_ID: return QPixmap((const char**)image4_data);
        case image5_ID: return QPixmap((const char**)image5_data);
        default: return QPixmap();
    } // switch
    } // icon

};

namespace Ui {
    class QG_CadToolBarCircles: public Ui_QG_CadToolBarCircles {};
} // namespace Ui

QT_END_NAMESPACE

class QG_CadToolBarCircles : public QWidget, public Ui::QG_CadToolBarCircles
{
    Q_OBJECT

public:
    QG_CadToolBarCircles(QWidget* parent = 0, const char* name = 0, Qt::WindowFlags fl = 0);
    ~QG_CadToolBarCircles();

public slots:
    virtual void mousePressEvent( QMouseEvent * e );
    virtual void contextMenuEvent( QContextMenuEvent * e );
    virtual void setCadToolBar( QG_CadToolBar * tb );
    virtual void drawCircle();
    virtual void drawCircleCR();
    virtual void drawCircle2P();
    virtual void drawCircle3P();
    virtual void drawCircleParallel();
    virtual void back();

protected:
    QG_ActionHandler* actionHandler;
    QG_CadToolBar* cadToolBar;

protected slots:
    virtual void languageChange();

private:
    void init();

};

#endif // QG_CADTOOLBARCIRCLES_H
