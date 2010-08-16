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
#ifndef QG_CADTOOLBARPOLYLINES_H
#define QG_CADTOOLBARPOLYLINES_H

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

class Ui_QG_CadToolBarPolylines
{
public:
    QToolButton *bBack;
    QToolButton *bPolyline;
    QToolButton *bPolylineTrim;
    QToolButton *bPolylineDelBetween;
    QToolButton *bPolylineDel;
    QToolButton *bPolylineAdd;
    QToolButton *bPolylineAppend;

    void setupUi(QWidget *QG_CadToolBarPolylines)
    {
        if (QG_CadToolBarPolylines->objectName().isEmpty())
            QG_CadToolBarPolylines->setObjectName(QString::fromUtf8("QG_CadToolBarPolylines"));
        QG_CadToolBarPolylines->resize(56, 336);
        QSizePolicy sizePolicy(static_cast<QSizePolicy::Policy>(0), static_cast<QSizePolicy::Policy>(1));
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(QG_CadToolBarPolylines->sizePolicy().hasHeightForWidth());
        QG_CadToolBarPolylines->setSizePolicy(sizePolicy);
        QG_CadToolBarPolylines->setMinimumSize(QSize(56, 336));
        bBack = new QToolButton(QG_CadToolBarPolylines);
        bBack->setObjectName(QString::fromUtf8("bBack"));
        bBack->setGeometry(QRect(0, 0, 56, 20));
        bBack->setIcon(qt_get_icon(image0_ID));
        bPolyline = new QToolButton(QG_CadToolBarPolylines);
        bPolyline->setObjectName(QString::fromUtf8("bPolyline"));
        bPolyline->setGeometry(QRect(0, 20, 28, 28));
        bPolyline->setIcon(qt_get_icon(image1_ID));
        bPolylineTrim = new QToolButton(QG_CadToolBarPolylines);
        bPolylineTrim->setObjectName(QString::fromUtf8("bPolylineTrim"));
        bPolylineTrim->setGeometry(QRect(28, 76, 28, 28));
        bPolylineTrim->setIcon(qt_get_icon(image2_ID));
        bPolylineDelBetween = new QToolButton(QG_CadToolBarPolylines);
        bPolylineDelBetween->setObjectName(QString::fromUtf8("bPolylineDelBetween"));
        bPolylineDelBetween->setGeometry(QRect(0, 76, 28, 28));
        bPolylineDelBetween->setIcon(qt_get_icon(image3_ID));
        bPolylineDel = new QToolButton(QG_CadToolBarPolylines);
        bPolylineDel->setObjectName(QString::fromUtf8("bPolylineDel"));
        bPolylineDel->setGeometry(QRect(28, 48, 28, 28));
        bPolylineDel->setIcon(qt_get_icon(image4_ID));
        bPolylineAdd = new QToolButton(QG_CadToolBarPolylines);
        bPolylineAdd->setObjectName(QString::fromUtf8("bPolylineAdd"));
        bPolylineAdd->setGeometry(QRect(28, 20, 28, 28));
        bPolylineAdd->setIcon(qt_get_icon(image5_ID));
        bPolylineAppend = new QToolButton(QG_CadToolBarPolylines);
        bPolylineAppend->setObjectName(QString::fromUtf8("bPolylineAppend"));
        bPolylineAppend->setGeometry(QRect(0, 48, 28, 28));
        bPolylineAppend->setIcon(qt_get_icon(image6_ID));

        retranslateUi(QG_CadToolBarPolylines);
        QObject::connect(bPolyline, SIGNAL(clicked()), QG_CadToolBarPolylines, SLOT(drawPolyline()));
        QObject::connect(bBack, SIGNAL(clicked()), QG_CadToolBarPolylines, SLOT(back()));
        QObject::connect(bPolylineAdd, SIGNAL(clicked()), QG_CadToolBarPolylines, SLOT(polylineAdd()));
        QObject::connect(bPolylineDel, SIGNAL(clicked()), QG_CadToolBarPolylines, SLOT(polylineDel()));
        QObject::connect(bPolylineDelBetween, SIGNAL(clicked()), QG_CadToolBarPolylines, SLOT(polylineDelBetween()));
        QObject::connect(bPolylineTrim, SIGNAL(clicked()), QG_CadToolBarPolylines, SLOT(polylineTrim()));
        QObject::connect(bPolylineAppend, SIGNAL(clicked()), QG_CadToolBarPolylines, SLOT(polylineAppend()));

        QMetaObject::connectSlotsByName(QG_CadToolBarPolylines);
    } // setupUi

    void retranslateUi(QWidget *QG_CadToolBarPolylines)
    {
        QG_CadToolBarPolylines->setWindowTitle(QApplication::translate("QG_CadToolBarPolylines", "Polylines", 0, QApplication::UnicodeUTF8));
        bBack->setText(QString());
#ifndef QT_NO_TOOLTIP
        bBack->setProperty("toolTip", QVariant(QApplication::translate("QG_CadToolBarPolylines", "Back to main menu", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
        bPolyline->setText(QString());
#ifndef QT_NO_TOOLTIP
        bPolyline->setProperty("toolTip", QVariant(QApplication::translate("QG_CadToolBarPolylines", "Create Polyline", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
        bPolylineTrim->setText(QString());
#ifndef QT_NO_TOOLTIP
        bPolylineTrim->setProperty("toolTip", QVariant(QApplication::translate("QG_CadToolBarPolylines", "Trim segments", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
        bPolylineDelBetween->setText(QString());
#ifndef QT_NO_TOOLTIP
        bPolylineDelBetween->setProperty("toolTip", QVariant(QApplication::translate("QG_CadToolBarPolylines", "Delete between two nodes", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
        bPolylineDel->setText(QString());
#ifndef QT_NO_TOOLTIP
        bPolylineDel->setProperty("toolTip", QVariant(QApplication::translate("QG_CadToolBarPolylines", "Delete node", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
        bPolylineAdd->setText(QString());
#ifndef QT_NO_TOOLTIP
        bPolylineAdd->setProperty("toolTip", QVariant(QApplication::translate("QG_CadToolBarPolylines", "Add node", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
        bPolylineAppend->setText(QString());
#ifndef QT_NO_TOOLTIP
        bPolylineAppend->setProperty("toolTip", QVariant(QApplication::translate("QG_CadToolBarPolylines", "Append node", 0, QApplication::UnicodeUTF8)));
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
"a c #000000",
"# c #ff0000",
"..................",
"..................",
"..................",
"..................",
"..................",
"......#...........",
"....a###..........",
"...a..#...........",
"..a...............",
"..a...............",
"..a...............",
"...a..#........#..",
"....a###aaaaaa###.",
"......#........#..",
"..................",
"..................",
"..................",
".................."};


    static const char* const image2_data[] = { 
"18 18 4 1",
". c None",
"b c #000000",
"a c #0000ff",
"# c #ff0000",
"..................",
"..................",
"..............##..",
"............##.#..",
"..........##...#..",
".......a##.....#..",
"......aaa......#..",
"....bb.a.......#..",
"..bb...b.......#..",
"bb.....b.......#..",
".......a.......a..",
"......aaabbbbbaaa.",
".......a.......a..",
"...............b..",
"...............b..",
"...............b..",
"...............b..",
"...............b.."};


    static const char* const image3_data[] = { 
"18 18 5 1",
". c None",
"b c #000000",
"a c #0000ff",
"# c #ff0000",
"c c #ffffff",
"..................",
"..................",
"..................",
".#....a.....a...#.",
"###bbaaa...aaab###",
".#....a.....a...#.",
"......b.....b.....",
"......b.....b.....",
"......a.....a.....",
".....aaabbbaaa....",
"......a.....a.....",
"..................",
"..................",
".ccccc............",
"cbbbbbc...........",
".ccccc............",
"..................",
".................."};


    static const char* const image4_data[] = { 
"18 18 4 1",
". c None",
"a c #000000",
"# c #ff0000",
"b c #ffffff",
"..................",
"..................",
"..................",
".#......#.......#.",
"###aaaa###aaaaa###",
".#......#a......#.",
".........aa.......",
".........aaa......",
".........aaaa.....",
".........aaaaa....",
".........aaaaaa...",
".........aaaa.....",
".........a.aa.....",
".bbbbb......aa....",
"baaaaab.....aa....",
".bbbbb............",
"..................",
".................."};


    static const char* const image5_data[] = { 
"18 18 4 1",
". c None",
"a c #000000",
"# c #ff0000",
"b c #ffffff",
"..................",
"..................",
"..................",
".#......#.......#.",
"###aaaa###aaaaa###",
".#......#a......#.",
".........aa.......",
".........aaa......",
".........aaaa.....",
".........aaaaa....",
".........aaaaaa...",
"...b.....aaaa.....",
"..bab....a.aa.....",
".bbabb......aa....",
"baaaaab.....aa....",
".bbabb............",
"..bab.............",
"...b.............."};


    static const char* const image6_data[] = { 
"18 18 4 1",
". c None",
"a c #000000",
"# c #ff0000",
"b c #ffffff",
"..................",
"..................",
"..................",
".#......#.........",
"###aaaa###........",
".#......#a........",
"..........a.......",
"...........a#.....",
"...........#a#....",
"............aa....",
"............aaa...",
"...b........aaaa..",
"..bab.......aaaaa.",
".bbabb......aaaaaa",
"baaaaab.....aaaa..",
".bbabb......a.aa..",
"..bab..........aa.",
"...b...........aa."};


    switch (id) {
        case image0_ID: return QPixmap((const char**)image0_data);
        case image1_ID: return QPixmap((const char**)image1_data);
        case image2_ID: return QPixmap((const char**)image2_data);
        case image3_ID: return QPixmap((const char**)image3_data);
        case image4_ID: return QPixmap((const char**)image4_data);
        case image5_ID: return QPixmap((const char**)image5_data);
        case image6_ID: return QPixmap((const char**)image6_data);
        default: return QPixmap();
    } // switch
    } // icon

};

namespace Ui {
    class QG_CadToolBarPolylines: public Ui_QG_CadToolBarPolylines {};
} // namespace Ui

QT_END_NAMESPACE

class QG_CadToolBarPolylines : public QWidget, public Ui::QG_CadToolBarPolylines
{
    Q_OBJECT

public:
    QG_CadToolBarPolylines(QWidget* parent = 0, const char* name = 0, Qt::WindowFlags fl = 0);
    ~QG_CadToolBarPolylines();

public slots:
    virtual void mousePressEvent( QMouseEvent * e );
    virtual void contextMenuEvent( QContextMenuEvent * e );
    virtual void setCadToolBar( QG_CadToolBar * tb );
    virtual void drawPolyline();
    virtual void polylineAdd();
    virtual void polylineAppend();
    virtual void polylineDel();
    virtual void polylineDelBetween();
    virtual void polylineTrim();
    virtual void back();

protected:
    QG_ActionHandler* actionHandler;
    QG_CadToolBar* cadToolBar;

protected slots:
    virtual void languageChange();

private:
    void init();

};

#endif // QG_CADTOOLBARPOLYLINES_H
