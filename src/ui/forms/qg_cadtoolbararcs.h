/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2010 R. van Twisk (librecad@rvt.dds.nl)
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
#ifndef QG_CADTOOLBARARCS_H
#define QG_CADTOOLBARARCS_H

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

class Ui_QG_CadToolBarArcs
{
public:
    QToolButton *bBack;
    QToolButton *bArcParallel;
    QToolButton *bArc;
    QToolButton *bArc3P;
    QToolButton *bArcTangential;

    void setupUi(QWidget *QG_CadToolBarArcs)
    {
        if (QG_CadToolBarArcs->objectName().isEmpty())
            QG_CadToolBarArcs->setObjectName(QString::fromUtf8("QG_CadToolBarArcs"));
        QG_CadToolBarArcs->resize(56, 336);
        QSizePolicy sizePolicy(static_cast<QSizePolicy::Policy>(0), static_cast<QSizePolicy::Policy>(1));
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(QG_CadToolBarArcs->sizePolicy().hasHeightForWidth());
        QG_CadToolBarArcs->setSizePolicy(sizePolicy);
        QG_CadToolBarArcs->setMinimumSize(QSize(56, 336));
        bBack = new QToolButton(QG_CadToolBarArcs);
        bBack->setObjectName(QString::fromUtf8("bBack"));
        bBack->setGeometry(QRect(0, 0, 56, 20));
        bBack->setIcon(qt_get_icon(image0_ID));
        bArcParallel = new QToolButton(QG_CadToolBarArcs);
        bArcParallel->setObjectName(QString::fromUtf8("bArcParallel"));
        bArcParallel->setGeometry(QRect(0, 48, 28, 28));
        bArcParallel->setIcon(qt_get_icon(image1_ID));
        bArc = new QToolButton(QG_CadToolBarArcs);
        bArc->setObjectName(QString::fromUtf8("bArc"));
        bArc->setGeometry(QRect(0, 20, 28, 28));
        bArc->setIcon(qt_get_icon(image2_ID));
        bArc3P = new QToolButton(QG_CadToolBarArcs);
        bArc3P->setObjectName(QString::fromUtf8("bArc3P"));
        bArc3P->setGeometry(QRect(28, 20, 28, 28));
        bArc3P->setIcon(qt_get_icon(image3_ID));
        bArcTangential = new QToolButton(QG_CadToolBarArcs);
        bArcTangential->setObjectName(QString::fromUtf8("bArcTangential"));
        bArcTangential->setGeometry(QRect(28, 48, 28, 28));
        bArcTangential->setIcon(qt_get_icon(image4_ID));
        QWidget::setTabOrder(bBack, bArc);
        QWidget::setTabOrder(bArc, bArc3P);
        QWidget::setTabOrder(bArc3P, bArcParallel);

        retranslateUi(QG_CadToolBarArcs);
        QObject::connect(bArc, SIGNAL(clicked()), QG_CadToolBarArcs, SLOT(drawArc()));
        QObject::connect(bArc3P, SIGNAL(clicked()), QG_CadToolBarArcs, SLOT(drawArc3P()));
        QObject::connect(bArcParallel, SIGNAL(clicked()), QG_CadToolBarArcs, SLOT(drawArcParallel()));
        QObject::connect(bBack, SIGNAL(clicked()), QG_CadToolBarArcs, SLOT(back()));
        QObject::connect(bArcTangential, SIGNAL(clicked()), QG_CadToolBarArcs, SLOT(drawArcTangential()));

        QMetaObject::connectSlotsByName(QG_CadToolBarArcs);
    } // setupUi

    void retranslateUi(QWidget *QG_CadToolBarArcs)
    {
        QG_CadToolBarArcs->setWindowTitle(QApplication::translate("QG_CadToolBarArcs", "Arcs", 0, QApplication::UnicodeUTF8));
        bBack->setText(QString());
#ifndef QT_NO_TOOLTIP
        bBack->setProperty("toolTip", QVariant(QApplication::translate("QG_CadToolBarArcs", "Back to main menu", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
        bArcParallel->setText(QString());
#ifndef QT_NO_TOOLTIP
        bArcParallel->setProperty("toolTip", QVariant(QApplication::translate("QG_CadToolBarArcs", "Concentric", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
        bArc->setText(QString());
#ifndef QT_NO_TOOLTIP
        bArc->setProperty("toolTip", QVariant(QApplication::translate("QG_CadToolBarArcs", "Arc with Center, Point, Angles", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
        bArc3P->setText(QString());
#ifndef QT_NO_TOOLTIP
        bArc3P->setProperty("toolTip", QVariant(QApplication::translate("QG_CadToolBarArcs", "Arc with three points", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
        bArcTangential->setText(QString());
#ifndef QT_NO_TOOLTIP
        bArcTangential->setProperty("toolTip", QVariant(QApplication::translate("QG_CadToolBarArcs", "Arc tangential to base entity with radius", 0, QApplication::UnicodeUTF8)));
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
"17 15 3 1",
". c None",
"# c #000000",
"a c #ff0000",
".............####",
".........####....",
".......##........",
"......#..........",
"....##.......aaaa",
"...#......aaa....",
"..#.....aa.......",
"..#....a.........",
".#....a.......###",
"#....a.....###...",
"....a.....#......",
"....a....#.......",
"...a....#........",
".......#.........",
".......#........."};


    static const char* const image2_data[] = { 
"18 18 3 1",
". c None",
"a c #000000",
"# c #ff0000",
"...............#..",
"...............#..",
"............aaa#..",
".........aaa...#..",
".......aa......#..",
"......a...........",
".....a............",
"....a.............",
"...a..............",
"#..a..............",
".#a...............",
"..##..............",
"....#.............",
"..................",
"...............#..",
"..............###.",
"...............#..",
".................."};


    static const char* const image3_data[] = { 
"18 18 3 1",
". c None",
"a c #000000",
"# c #ff0000",
".......#.aaaaaa.#.",
"......###......###",
".....aa#........#.",
"....a.............",
"...a..............",
"..a...............",
"..a...............",
".a................",
".a................",
"a.................",
"a.................",
"a.................",
"a.................",
"a.................",
"a.................",
".#................",
"###...............",
".#................"};


    static const char* const image4_data[] = { 
"18 18 3 1",
". c None",
"# c #000000",
"a c #ff0000",
"..................",
"..................",
"..................",
"..................",
"..................",
"........####......",
"......a#....##....",
"....aa.......#....",
"..aa..........#...",
"aa.......a....#...",
"..........a...#...",
"...........a..#...",
"............a#....",
"........a...##....",
".......aaa##......",
"........a.........",
"..................",
".................."};


    switch (id) {
        case image0_ID: return QPixmap((const char**)image0_data);
        case image1_ID: return QPixmap((const char**)image1_data);
        case image2_ID: return QPixmap((const char**)image2_data);
        case image3_ID: return QPixmap((const char**)image3_data);
        case image4_ID: return QPixmap((const char**)image4_data);
        default: return QPixmap();
    } // switch
    } // icon

};

namespace Ui {
    class QG_CadToolBarArcs: public Ui_QG_CadToolBarArcs {};
} // namespace Ui

QT_END_NAMESPACE

class QG_CadToolBarArcs : public QWidget, public Ui::QG_CadToolBarArcs
{
    Q_OBJECT

public:
    QG_CadToolBarArcs(QWidget* parent = 0, const char* name = 0, Qt::WindowFlags fl = 0);
    ~QG_CadToolBarArcs();

public slots:
    virtual void contextMenuEvent( QContextMenuEvent * e );
    virtual void setCadToolBar( QG_CadToolBar * tb );
    virtual void drawArc();
    virtual void drawArc3P();
    virtual void drawArcParallel();
    virtual void drawArcTangential();
    virtual void back();

protected:
    QG_CadToolBar* cadToolBar;
    QG_ActionHandler* actionHandler;

protected slots:
    virtual void languageChange();

private:
    void init();

};

#endif // QG_CADTOOLBARARCS_H
