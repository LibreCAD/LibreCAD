/****************************************************************************
**
** This file is part of the CADuntu project, a 2D CAD program
**
** Copyright (C) 2010 R. van Twisk (caduntu@rvt.dds.nl)
** Copyright (C) 2001-2003 RibbonSoft. All rights reserved.
**
**
** This file may be distributed and/or modified under the terms of the
** GNU General Public License version 2 as published by the Free Software 
** Foundation and appearing in the file gpl-2.0.txt included in the
** packaging of this file.
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
#ifndef QG_CADTOOLBARSELECT_H
#define QG_CADTOOLBARSELECT_H

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
#include "rs_actioninterface.h"

QT_BEGIN_NAMESPACE

class Ui_QG_CadToolBarSelect
{
public:
    QToolButton *bAll;
    QToolButton *bBack;
    QToolButton *bInters;
    QToolButton *bUnInters;
    QToolButton *bUnAll;
    QToolButton *bInvert;
    QToolButton *bLayer;
    QToolButton *bContour;
    QToolButton *bSingle;
    QToolButton *bUnWindow;
    QToolButton *bWindow;
    QToolButton *bDoit;

    void setupUi(QWidget *QG_CadToolBarSelect)
    {
        if (QG_CadToolBarSelect->objectName().isEmpty())
            QG_CadToolBarSelect->setObjectName(QString::fromUtf8("QG_CadToolBarSelect"));
        QG_CadToolBarSelect->resize(56, 336);
        QSizePolicy sizePolicy(static_cast<QSizePolicy::Policy>(0), static_cast<QSizePolicy::Policy>(1));
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(QG_CadToolBarSelect->sizePolicy().hasHeightForWidth());
        QG_CadToolBarSelect->setSizePolicy(sizePolicy);
        QG_CadToolBarSelect->setMinimumSize(QSize(56, 336));
        bAll = new QToolButton(QG_CadToolBarSelect);
        bAll->setObjectName(QString::fromUtf8("bAll"));
        bAll->setGeometry(QRect(28, 20, 28, 28));
        bAll->setIcon(qt_get_icon(image0_ID));
        bBack = new QToolButton(QG_CadToolBarSelect);
        bBack->setObjectName(QString::fromUtf8("bBack"));
        bBack->setGeometry(QRect(0, 0, 56, 20));
        bBack->setIcon(qt_get_icon(image1_ID));
        bInters = new QToolButton(QG_CadToolBarSelect);
        bInters->setObjectName(QString::fromUtf8("bInters"));
        bInters->setGeometry(QRect(28, 104, 28, 28));
        bInters->setIcon(qt_get_icon(image2_ID));
        bUnInters = new QToolButton(QG_CadToolBarSelect);
        bUnInters->setObjectName(QString::fromUtf8("bUnInters"));
        bUnInters->setGeometry(QRect(0, 104, 28, 28));
        bUnInters->setIcon(qt_get_icon(image3_ID));
        bUnAll = new QToolButton(QG_CadToolBarSelect);
        bUnAll->setObjectName(QString::fromUtf8("bUnAll"));
        bUnAll->setGeometry(QRect(0, 20, 28, 28));
        bUnAll->setIcon(qt_get_icon(image4_ID));
        bInvert = new QToolButton(QG_CadToolBarSelect);
        bInvert->setObjectName(QString::fromUtf8("bInvert"));
        bInvert->setGeometry(QRect(0, 132, 28, 28));
        bInvert->setIcon(qt_get_icon(image5_ID));
        bLayer = new QToolButton(QG_CadToolBarSelect);
        bLayer->setObjectName(QString::fromUtf8("bLayer"));
        bLayer->setGeometry(QRect(28, 132, 28, 28));
        bLayer->setIcon(qt_get_icon(image6_ID));
        bContour = new QToolButton(QG_CadToolBarSelect);
        bContour->setObjectName(QString::fromUtf8("bContour"));
        bContour->setGeometry(QRect(28, 48, 28, 28));
        bContour->setIcon(qt_get_icon(image7_ID));
        bSingle = new QToolButton(QG_CadToolBarSelect);
        bSingle->setObjectName(QString::fromUtf8("bSingle"));
        bSingle->setGeometry(QRect(0, 48, 28, 28));
        bSingle->setIcon(qt_get_icon(image8_ID));
        bUnWindow = new QToolButton(QG_CadToolBarSelect);
        bUnWindow->setObjectName(QString::fromUtf8("bUnWindow"));
        bUnWindow->setGeometry(QRect(0, 76, 28, 28));
        bUnWindow->setIcon(qt_get_icon(image9_ID));
        bWindow = new QToolButton(QG_CadToolBarSelect);
        bWindow->setObjectName(QString::fromUtf8("bWindow"));
        bWindow->setGeometry(QRect(28, 76, 28, 28));
        bWindow->setIcon(qt_get_icon(image10_ID));
        bDoit = new QToolButton(QG_CadToolBarSelect);
        bDoit->setObjectName(QString::fromUtf8("bDoit"));
        bDoit->setGeometry(QRect(0, 160, 56, 20));
        bDoit->setIcon(qt_get_icon(image11_ID));

        retranslateUi(QG_CadToolBarSelect);
        QObject::connect(bSingle, SIGNAL(clicked()), QG_CadToolBarSelect, SLOT(selectSingle()));
        QObject::connect(bDoit, SIGNAL(clicked()), QG_CadToolBarSelect, SLOT(runNextAction()));
        QObject::connect(bBack, SIGNAL(clicked()), QG_CadToolBarSelect, SLOT(back()));
        QObject::connect(bAll, SIGNAL(clicked()), QG_CadToolBarSelect, SLOT(selectAll()));
        QObject::connect(bWindow, SIGNAL(clicked()), QG_CadToolBarSelect, SLOT(selectWindow()));
        QObject::connect(bUnAll, SIGNAL(clicked()), QG_CadToolBarSelect, SLOT(deselectAll()));
        QObject::connect(bUnWindow, SIGNAL(clicked()), QG_CadToolBarSelect, SLOT(deselectWindow()));
        QObject::connect(bContour, SIGNAL(clicked()), QG_CadToolBarSelect, SLOT(selectContour()));
        QObject::connect(bUnInters, SIGNAL(clicked()), QG_CadToolBarSelect, SLOT(deselectIntersected()));
        QObject::connect(bInters, SIGNAL(clicked()), QG_CadToolBarSelect, SLOT(selectIntersected()));
        QObject::connect(bInvert, SIGNAL(clicked()), QG_CadToolBarSelect, SLOT(selectInvert()));
        QObject::connect(bLayer, SIGNAL(clicked()), QG_CadToolBarSelect, SLOT(selectLayer()));

        QMetaObject::connectSlotsByName(QG_CadToolBarSelect);
    } // setupUi

    void retranslateUi(QWidget *QG_CadToolBarSelect)
    {
        QG_CadToolBarSelect->setWindowTitle(QApplication::translate("QG_CadToolBarSelect", "Select", 0, QApplication::UnicodeUTF8));
        bAll->setText(QString());
#ifndef QT_NO_TOOLTIP
        bAll->setProperty("toolTip", QVariant(QApplication::translate("QG_CadToolBarSelect", "Select all", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
        bBack->setText(QString());
#ifndef QT_NO_TOOLTIP
        bBack->setProperty("toolTip", QVariant(QApplication::translate("QG_CadToolBarSelect", "Back to main menu", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
        bInters->setText(QString());
#ifndef QT_NO_TOOLTIP
        bInters->setProperty("toolTip", QVariant(QApplication::translate("QG_CadToolBarSelect", "Select intersected entities", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
        bUnInters->setText(QString());
#ifndef QT_NO_TOOLTIP
        bUnInters->setProperty("toolTip", QVariant(QApplication::translate("QG_CadToolBarSelect", "Deselect intersected entities", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
        bUnAll->setText(QString());
#ifndef QT_NO_TOOLTIP
        bUnAll->setProperty("toolTip", QVariant(QApplication::translate("QG_CadToolBarSelect", "Deselect all", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
        bInvert->setText(QString());
#ifndef QT_NO_TOOLTIP
        bInvert->setProperty("toolTip", QVariant(QApplication::translate("QG_CadToolBarSelect", "Invert Selection", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
        bLayer->setText(QString());
#ifndef QT_NO_TOOLTIP
        bLayer->setProperty("toolTip", QVariant(QApplication::translate("QG_CadToolBarSelect", "Select layer", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
        bContour->setText(QString());
#ifndef QT_NO_TOOLTIP
        bContour->setProperty("toolTip", QVariant(QApplication::translate("QG_CadToolBarSelect", "(De-)Select contour", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
        bSingle->setText(QString());
#ifndef QT_NO_TOOLTIP
        bSingle->setProperty("toolTip", QVariant(QApplication::translate("QG_CadToolBarSelect", "(De-)Select entity", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
        bUnWindow->setText(QString());
#ifndef QT_NO_TOOLTIP
        bUnWindow->setProperty("toolTip", QVariant(QApplication::translate("QG_CadToolBarSelect", "Deselect Window", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
        bWindow->setText(QString());
#ifndef QT_NO_TOOLTIP
        bWindow->setProperty("toolTip", QVariant(QApplication::translate("QG_CadToolBarSelect", "Select Window", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
        bDoit->setText(QString());
#ifndef QT_NO_TOOLTIP
        bDoit->setProperty("toolTip", QVariant(QApplication::translate("QG_CadToolBarSelect", "Continue action", 0, QApplication::UnicodeUTF8)));
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
        image9_ID,
        image10_ID,
        image11_ID,
        unknown_ID
    };
    static QPixmap qt_get_icon(IconID id)
    {
    static const char* const image0_data[] = { 
"18 16 2 1",
". c None",
"# c #ff0000",
".....#############",
"...##............#",
"..#..............#",
".#...............#",
".#...###.........#",
"#...#...#........#",
"#...#...#....#...#",
"#...#...#...##...#",
"#....###...#.#...#",
"#.........#..#...#",
"#........#...#...#",
"######..######...#",
".....#...........#",
".....#...........#",
".....#...........#",
".....#############"};


    static const char* const image1_data[] = { 
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


    static const char* const image2_data[] = { 
"15 18 4 1",
". c None",
"# c #000000",
"b c #0000ff",
"a c #ff0000",
".........#.....",
"........###....",
"....aaaaabaaaaa",
"..##.....b....#",
".#.......b....#",
".#..###..b....#",
"#..#...#.b....#",
"#..#...#.b....#",
"#..#...#.b.a..#",
"#...###..ba#..#",
"#........b.#..#",
"#.......ab.#..#",
"#####..aabaa..#",
"....#....b....#",
"....#....b....#",
"....aaaaabaaaaa",
"........###....",
".........#....."};


    static const char* const image3_data[] = { 
"15 18 4 1",
". c None",
"# c #000000",
"b c #0000ff",
"a c #ff0000",
".........#.....",
"........###....",
"....a####b#####",
"..aa.....b....a",
".a.......b....a",
".a..aaa..b....a",
"a..a...a.b....a",
"a..a...a.b....a",
"a..a...a.b.#..a",
"a...aaa..b#a..a",
"a........b.a..a",
"a.......#b.a..a",
"aaaaa..##b##..a",
"....a....b....a",
"....a....b....a",
"....#####b#####",
"........###....",
".........#....."};


    static const char* const image4_data[] = { 
"18 16 2 1",
". c None",
"# c #000000",
".....#############",
"...##............#",
"..#..............#",
".#...............#",
".#...###.........#",
"#...#...#........#",
"#...#...#....#...#",
"#...#...#...##...#",
"#....###...#.#...#",
"#.........#..#...#",
"#........#...#...#",
"######..######...#",
".....#...........#",
".....#...........#",
".....#...........#",
".....#############"};


    static const char* const image5_data[] = { 
"18 16 4 1",
"# c None",
"b c #000000",
". c #0000ff",
"a c #ff0000",
".####aaaaaaaaaaa##",
"#.#aa##########a##",
"##.############a##",
"##a.#bbb#######a##",
"#b##..##b######a##",
"#b##a#.#b######a##",
"#b##a##.b###b##a##",
"#b###aaa.##bb##a##",
"#b#######.b#b##a##",
"#b#######a.#b##a##",
"#bbbbb##aaa.b##a##",
"#####b######..#a##",
"#####b########.a##",
"#####bbbbbbbbbb.##",
"################.#",
"#################."};


    static const char* const image6_data[] = { 
"14 19 4 1",
". c None",
"# c #000000",
"b c #ff0000",
"a c #ffffff",
"........##....",
"......##a#....",
"....##aaa#..##",
"..##aaa#a###a#",
"##aaa##a##aaa#",
"#aa##a##aaaaa#",
"#a#a##aaaabaa#",
"#a#a#aaaaabaa#",
"#a#a#aaaabbaa#",
"#a#a#aaaabbaa#",
"#a#a#aaababaa#",
"#a###aaababaa#",
"#a#a#aababbaa#",
"#aaa#aabbaaaa#",
"#aaa#abaaaaa##",
"#a###aaaaa##..",
"##..#aaa##....",
"....#a##......",
"....##........"};


    static const char* const image7_data[] = { 
"18 16 3 1",
". c None",
"a c #000000",
"# c #ff0000",
".....#############",
"...##............#",
"..#..............#",
".#...............#",
".#...aaa.........#",
"#...a...a........#",
"#...a...a....a...#",
"#...a...a...aa...#",
"#....aaa...a.a...#",
"#.........a..a...#",
"#........a...a...#",
"######..aaaaaa...#",
".....#...........#",
".....#...........#",
".....#...........#",
".....#############"};


    static const char* const image8_data[] = { 
"15 18 4 1",
". c None",
"a c #000000",
"# c #ff0000",
"b c #ffffff",
"..............#",
"............##.",
"..........##...",
"........##.....",
".......#.......",
".....##a.......",
"...##..aa......",
".##....aba.....",
"#......abba....",
".......abbba...",
".......abbbba..",
".......abbbaaa.",
".......ababa...",
".......aaabba..",
".......a..aba..",
"...........aba.",
"...........aba.",
"............a.."};


    static const char* const image9_data[] = { 
"18 17 3 1",
". c None",
"# c #000000",
"a c #0000ff",
".#................",
"###aaaaaaaaaaaaaa.",
".#..............a.",
".a..............a.",
".a...##.........a.",
".a..#..#........a.",
".a.#....#.......a.",
".a.#....#....#..a.",
".a..#..#....##..a.",
".a...##....#.#..a.",
".a........#..#..a.",
".a.......#...#..a.",
".a......######..a.",
".a..............a.",
".a..............#.",
".aaaaaaaaaaaaaa###",
"................#."};


    static const char* const image10_data[] = { 
"18 16 4 1",
". c None",
"# c #000000",
"a c #0000ff",
"b c #ff0000",
".#................",
"###aaaaaaaaaaaaaa.",
".#..............a.",
".a..............a.",
".a...bbb........a.",
".a..b...b.......a.",
".a..b...b....b..a.",
".a..b...b...bb..a.",
".a...bbb...b.b..a.",
".a........b..b..a.",
".a.......b...b..a.",
".a......bbbbbb..a.",
".a..............a.",
".a..............#.",
".aaaaaaaaaaaaaa###",
"................#."};


    static const char* const image11_data[] = { 
"16 11 3 1",
". c None",
"a c #000000",
"# c #ffffff",
".......#a.......",
".......#aa......",
"########aaa.....",
"#aaaaaaaaaaa....",
"#aaaaaaaaaaaa...",
"#aaaaaaaaaaaaa..",
"#aaaaaaaaaaaa...",
".aaaaaaaaaaa....",
".......#aaa.....",
".......#aa......",
"........a......."};


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
        case image9_ID: return QPixmap((const char**)image9_data);
        case image10_ID: return QPixmap((const char**)image10_data);
        case image11_ID: return QPixmap((const char**)image11_data);
        default: return QPixmap();
    } // switch
    } // icon

};

namespace Ui {
    class QG_CadToolBarSelect: public Ui_QG_CadToolBarSelect {};
} // namespace Ui

QT_END_NAMESPACE

class QG_CadToolBarSelect : public QWidget, public Ui::QG_CadToolBarSelect
{
    Q_OBJECT

public:
    QG_CadToolBarSelect(QWidget* parent = 0, const char* name = 0, Qt::WindowFlags fl = 0);
    ~QG_CadToolBarSelect();

public slots:
    virtual void mousePressEvent( QMouseEvent * e );
    virtual void contextMenuEvent( QContextMenuEvent * e );
    virtual void setCadToolBar( QG_CadToolBar * tb );
    virtual void selectSingle();
    virtual void selectContour();
    virtual void deselectAll();
    virtual void selectAll();
    virtual void selectWindow();
    virtual void deselectWindow();
    virtual void selectIntersected();
    virtual void deselectIntersected();
    virtual void selectInvert();
    virtual void selectLayer();
    virtual void setSelectAction( RS_ActionInterface * selectAction );
    virtual void setNextAction( int nextAction );
    virtual void runNextAction();
    virtual void back();

protected:
    QG_CadToolBar* cadToolBar;
    QG_ActionHandler* actionHandler;

protected slots:
    virtual void languageChange();

private:
    int nextAction;
    RS_ActionInterface* selectAction;

    void init();

};

#endif // QG_CADTOOLBARSELECT_H
