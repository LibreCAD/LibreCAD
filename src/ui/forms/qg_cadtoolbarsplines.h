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
#ifndef QG_CADTOOLBARSPLINES_H
#define QG_CADTOOLBARSPLINES_H

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

class Ui_QG_CadToolBarSplines
{
public:
    QToolButton *bBack;
    QToolButton *bSpline;

    void setupUi(QWidget *QG_CadToolBarSplines)
    {
        if (QG_CadToolBarSplines->objectName().isEmpty())
            QG_CadToolBarSplines->setObjectName(QString::fromUtf8("QG_CadToolBarSplines"));
        QG_CadToolBarSplines->resize(56, 336);
        QSizePolicy sizePolicy(static_cast<QSizePolicy::Policy>(0), static_cast<QSizePolicy::Policy>(1));
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(QG_CadToolBarSplines->sizePolicy().hasHeightForWidth());
        QG_CadToolBarSplines->setSizePolicy(sizePolicy);
        QG_CadToolBarSplines->setMinimumSize(QSize(56, 336));
        bBack = new QToolButton(QG_CadToolBarSplines);
        bBack->setObjectName(QString::fromUtf8("bBack"));
        bBack->setGeometry(QRect(0, 0, 56, 20));
        bBack->setIcon(qt_get_icon(image0_ID));
        bSpline = new QToolButton(QG_CadToolBarSplines);
        bSpline->setObjectName(QString::fromUtf8("bSpline"));
        bSpline->setGeometry(QRect(0, 20, 28, 28));
        bSpline->setIcon(qt_get_icon(image1_ID));

        retranslateUi(QG_CadToolBarSplines);
        QObject::connect(bSpline, SIGNAL(clicked()), QG_CadToolBarSplines, SLOT(drawSpline()));
        QObject::connect(bBack, SIGNAL(clicked()), QG_CadToolBarSplines, SLOT(back()));

        QMetaObject::connectSlotsByName(QG_CadToolBarSplines);
    } // setupUi

    void retranslateUi(QWidget *QG_CadToolBarSplines)
    {
        QG_CadToolBarSplines->setWindowTitle(QApplication::translate("QG_CadToolBarSplines", "Splines", 0, QApplication::UnicodeUTF8));
        bBack->setText(QString());
#ifndef QT_NO_TOOLTIP
        bBack->setProperty("toolTip", QVariant(QApplication::translate("QG_CadToolBarSplines", "Back to main menu", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
        bSpline->setText(QString());
#ifndef QT_NO_TOOLTIP
        bSpline->setProperty("toolTip", QVariant(QApplication::translate("QG_CadToolBarSplines", "Spline", 0, QApplication::UnicodeUTF8)));
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
"18 18 2 1",
". c None",
"# c #000000",
"..................",
"..................",
"..##..............",
".#..#.............",
".#...#......###...",
".#....#....#...#..",
"..#...#....#....#.",
"..#....#..#.....#.",
"..#.....#.#.....#.",
"...#.....##.....#.",
"...#......#.....#.",
"....#.....##...#..",
"....#....#..#..#..",
".....#...#...##...",
".....#...#........",
"......#..#........",
".......##.........",
".................."};


    switch (id) {
        case image0_ID: return QPixmap((const char**)image0_data);
        case image1_ID: return QPixmap((const char**)image1_data);
        default: return QPixmap();
    } // switch
    } // icon

};

namespace Ui {
    class QG_CadToolBarSplines: public Ui_QG_CadToolBarSplines {};
} // namespace Ui

QT_END_NAMESPACE

class QG_CadToolBarSplines : public QWidget, public Ui::QG_CadToolBarSplines
{
    Q_OBJECT

public:
    QG_CadToolBarSplines(QWidget* parent = 0, const char* name = 0, Qt::WindowFlags fl = 0);
    ~QG_CadToolBarSplines();

public slots:
    virtual void mousePressEvent( QMouseEvent * e );
    virtual void contextMenuEvent( QContextMenuEvent * e );
    virtual void setCadToolBar( QG_CadToolBar * tb );
    virtual void drawSpline();
    virtual void back();

protected:
    QG_ActionHandler* actionHandler;
    QG_CadToolBar* cadToolBar;

protected slots:
    virtual void languageChange();

private:
    void init();

};

#endif // QG_CADTOOLBARSPLINES_H
