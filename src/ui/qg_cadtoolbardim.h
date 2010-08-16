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
#ifndef QG_CADTOOLBARDIM_H
#define QG_CADTOOLBARDIM_H

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

class Ui_QG_CadToolBarDim
{
public:
    QToolButton *bBack;
    QToolButton *bAligned;
    QToolButton *bLinear;
    QToolButton *bLinearHor;
    QToolButton *bLinearVer;
    QToolButton *bRadial;
    QToolButton *bDiametric;
    QToolButton *bAngular;
    QToolButton *bLeader;

    void setupUi(QWidget *QG_CadToolBarDim)
    {
        if (QG_CadToolBarDim->objectName().isEmpty())
            QG_CadToolBarDim->setObjectName(QString::fromUtf8("QG_CadToolBarDim"));
        QG_CadToolBarDim->resize(56, 336);
        QSizePolicy sizePolicy(static_cast<QSizePolicy::Policy>(0), static_cast<QSizePolicy::Policy>(1));
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(QG_CadToolBarDim->sizePolicy().hasHeightForWidth());
        QG_CadToolBarDim->setSizePolicy(sizePolicy);
        QG_CadToolBarDim->setMinimumSize(QSize(56, 336));
        bBack = new QToolButton(QG_CadToolBarDim);
        bBack->setObjectName(QString::fromUtf8("bBack"));
        bBack->setGeometry(QRect(0, 0, 56, 20));
        bBack->setIcon(qt_get_icon(image0_ID));
        bAligned = new QToolButton(QG_CadToolBarDim);
        bAligned->setObjectName(QString::fromUtf8("bAligned"));
        bAligned->setGeometry(QRect(0, 20, 28, 28));
        bAligned->setIcon(qt_get_icon(image1_ID));
        bLinear = new QToolButton(QG_CadToolBarDim);
        bLinear->setObjectName(QString::fromUtf8("bLinear"));
        bLinear->setGeometry(QRect(28, 20, 28, 28));
        bLinear->setIcon(qt_get_icon(image2_ID));
        bLinearHor = new QToolButton(QG_CadToolBarDim);
        bLinearHor->setObjectName(QString::fromUtf8("bLinearHor"));
        bLinearHor->setGeometry(QRect(0, 48, 28, 28));
        bLinearHor->setIcon(qt_get_icon(image3_ID));
        bLinearVer = new QToolButton(QG_CadToolBarDim);
        bLinearVer->setObjectName(QString::fromUtf8("bLinearVer"));
        bLinearVer->setGeometry(QRect(28, 48, 28, 28));
        bLinearVer->setIcon(qt_get_icon(image4_ID));
        bRadial = new QToolButton(QG_CadToolBarDim);
        bRadial->setObjectName(QString::fromUtf8("bRadial"));
        bRadial->setGeometry(QRect(0, 76, 28, 28));
        bRadial->setIcon(qt_get_icon(image5_ID));
        bDiametric = new QToolButton(QG_CadToolBarDim);
        bDiametric->setObjectName(QString::fromUtf8("bDiametric"));
        bDiametric->setGeometry(QRect(28, 76, 28, 28));
        bDiametric->setIcon(qt_get_icon(image6_ID));
        bAngular = new QToolButton(QG_CadToolBarDim);
        bAngular->setObjectName(QString::fromUtf8("bAngular"));
        bAngular->setGeometry(QRect(0, 104, 28, 28));
        bAngular->setIcon(qt_get_icon(image7_ID));
        bLeader = new QToolButton(QG_CadToolBarDim);
        bLeader->setObjectName(QString::fromUtf8("bLeader"));
        bLeader->setGeometry(QRect(28, 104, 28, 28));
        bLeader->setIcon(qt_get_icon(image8_ID));

        retranslateUi(QG_CadToolBarDim);
        QObject::connect(bAligned, SIGNAL(clicked()), QG_CadToolBarDim, SLOT(drawDimAligned()));
        QObject::connect(bLinearHor, SIGNAL(clicked()), QG_CadToolBarDim, SLOT(drawDimLinearHor()));
        QObject::connect(bLinearVer, SIGNAL(clicked()), QG_CadToolBarDim, SLOT(drawDimLinearVer()));
        QObject::connect(bLinear, SIGNAL(clicked()), QG_CadToolBarDim, SLOT(drawDimLinear()));
        QObject::connect(bBack, SIGNAL(clicked()), QG_CadToolBarDim, SLOT(back()));
        QObject::connect(bRadial, SIGNAL(clicked()), QG_CadToolBarDim, SLOT(drawDimRadial()));
        QObject::connect(bDiametric, SIGNAL(clicked()), QG_CadToolBarDim, SLOT(drawDimDiametric()));
        QObject::connect(bAngular, SIGNAL(clicked()), QG_CadToolBarDim, SLOT(drawDimAngular()));
        QObject::connect(bLeader, SIGNAL(clicked()), QG_CadToolBarDim, SLOT(drawDimLeader()));

        QMetaObject::connectSlotsByName(QG_CadToolBarDim);
    } // setupUi

    void retranslateUi(QWidget *QG_CadToolBarDim)
    {
        QG_CadToolBarDim->setWindowTitle(QApplication::translate("QG_CadToolBarDim", "Dimensions", 0, QApplication::UnicodeUTF8));
        bBack->setText(QString());
#ifndef QT_NO_TOOLTIP
        bBack->setProperty("toolTip", QVariant(QApplication::translate("QG_CadToolBarDim", "Back to main menu", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
        bAligned->setText(QString());
#ifndef QT_NO_TOOLTIP
        bAligned->setProperty("toolTip", QVariant(QApplication::translate("QG_CadToolBarDim", "Aligned Dimension", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
        bLinear->setText(QString());
#ifndef QT_NO_TOOLTIP
        bLinear->setProperty("toolTip", QVariant(QApplication::translate("QG_CadToolBarDim", "Linear Dimension", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
        bLinearHor->setText(QString());
#ifndef QT_NO_TOOLTIP
        bLinearHor->setProperty("toolTip", QVariant(QApplication::translate("QG_CadToolBarDim", "Horizontal Dimension", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
        bLinearVer->setText(QString());
#ifndef QT_NO_TOOLTIP
        bLinearVer->setProperty("toolTip", QVariant(QApplication::translate("QG_CadToolBarDim", "Vertical Dimension", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
        bRadial->setText(QString());
#ifndef QT_NO_TOOLTIP
        bRadial->setProperty("toolTip", QVariant(QApplication::translate("QG_CadToolBarDim", "Radial Dimension", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
        bDiametric->setText(QString());
#ifndef QT_NO_TOOLTIP
        bDiametric->setProperty("toolTip", QVariant(QApplication::translate("QG_CadToolBarDim", "Diametric Dimension", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
        bAngular->setText(QString());
#ifndef QT_NO_TOOLTIP
        bAngular->setProperty("toolTip", QVariant(QApplication::translate("QG_CadToolBarDim", "Angular Dimension", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
        bLeader->setText(QString());
#ifndef QT_NO_TOOLTIP
        bLeader->setProperty("toolTip", QVariant(QApplication::translate("QG_CadToolBarDim", "Leader", 0, QApplication::UnicodeUTF8)));
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
        image6_ID,
        image7_ID,
        image8_ID,
        unknown_ID
    };
    static QPixmap qt_get_icon(IconID id)
    {
    static const char* const image0_data[] = { 
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


    static const char* const image1_data[] = { 
"18 18 3 1",
". c None",
"# c #000000",
"a c #ff0000",
"..........#.......",
"...........#......",
"............#.....",
".........###.#....",
"..........##..#...",
".........#.#...#..",
"........#.......a.",
".......#.......a..",
"......#.......a...",
"...#.#.......a....",
"#..##.......a.....",
".#.###.....a......",
"..#.......a.......",
"...#.....a........",
"....#...a.........",
".....#.a..........",
"......a...........",
".................."};


    static const char* const image2_data[] = { 
"18 16 2 1",
". c None",
"# c #000000",
"..........#.......",
"...........#......",
"............#.....",
".........###.#....",
"..........##..#...",
".........#.#...#..",
"........#.........",
".......#..........",
"......#...........",
"...#.#............",
"#..##.............",
".#.###............",
"..#...............",
"...#..............",
"....#.............",
".....#............"};


    static const char* const image3_data[] = { 
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


    static const char* const image4_data[] = { 
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


    static const char* const image5_data[] = { 
"18 13 2 1",
". c None",
"# c #000000",
"............#.....",
"........#..#......",
"........#.#.......",
"...####.##...####.",
"..#....#####.#...#",
".#.....##....#..#.",
"#.....#..#...###..",
"#....#...#...#..#.",
"#........#...#...#",
"#........#........",
".#......#.........",
"..#....#..........",
"...####..........."};


    static const char* const image6_data[] = { 
"15 15 2 1",
". c None",
"# c #000000",
".....#####.....",
"...##.....##...",
"..#.........#..",
".#........##.#.",
".#......####.#.",
"#........##...#",
"#.......#.#...#",
"#......#......#",
"#...#.#.......#",
"#...##........#",
".#.####......#.",
".#.##........#.",
"..#.........#..",
"...##.....##...",
".....#####....."};


    static const char* const image7_data[] = { 
"18 18 2 1",
". c None",
"# c #000000",
".........#........",
"........#.........",
".......###........",
"......#.####......",
".....#...##.......",
"....#....#.#......",
"...#.......#......",
"..#.........#.....",
".#..........#.....",
"#............#....",
".............###..",
"............####..",
".............###..",
".............##...",
"..............#...",
"##################",
"..................",
".................."};


    static const char* const image8_data[] = { 
"15 10 2 1",
". c None",
"# c #000000",
"...............",
".......########",
"......#........",
".....#.........",
"....#..........",
".#.#...........",
".##............",
"####...........",
"##.............",
"..............."};


    switch (id) {
        case image0_ID: return QPixmap((const char**)image0_data);
        case image1_ID: return QPixmap((const char**)image1_data);
        case image2_ID: return QPixmap((const char**)image2_data);
        case image3_ID: return QPixmap((const char**)image3_data);
        case image4_ID: return QPixmap((const char**)image4_data);
        case image5_ID: return QPixmap((const char**)image5_data);
        case image6_ID: return QPixmap((const char**)image6_data);
        case image7_ID: return QPixmap((const char**)image7_data);
        case image8_ID: return QPixmap((const char**)image8_data);
        default: return QPixmap();
    } // switch
    } // icon

};

namespace Ui {
    class QG_CadToolBarDim: public Ui_QG_CadToolBarDim {};
} // namespace Ui

QT_END_NAMESPACE

class QG_CadToolBarDim : public QWidget, public Ui::QG_CadToolBarDim
{
    Q_OBJECT

public:
    QG_CadToolBarDim(QWidget* parent = 0, const char* name = 0, Qt::WindowFlags fl = 0);
    ~QG_CadToolBarDim();

public slots:
    virtual void mousePressEvent( QMouseEvent * e );
    virtual void contextMenuEvent( QContextMenuEvent * e );
    virtual void setCadToolBar( QG_CadToolBar * tb );
    virtual void drawDimAligned();
    virtual void drawDimLinear();
    virtual void drawDimLinearHor();
    virtual void drawDimLinearVer();
    virtual void drawDimRadial();
    virtual void drawDimDiametric();
    virtual void drawDimAngular();
    virtual void drawDimLeader();
    virtual void back();

protected:
    QG_CadToolBar* cadToolBar;
    QG_ActionHandler* actionHandler;

protected slots:
    virtual void languageChange();

private:
    void init();

};

#endif // QG_CADTOOLBARDIM_H
