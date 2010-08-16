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
#ifndef QG_CADTOOLBARPOINTS_H
#define QG_CADTOOLBARPOINTS_H

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

class Ui_QG_CadToolBarPoints
{
public:
    QToolButton *bBack;
    QToolButton *bPoint;

    void setupUi(QWidget *QG_CadToolBarPoints)
    {
        if (QG_CadToolBarPoints->objectName().isEmpty())
            QG_CadToolBarPoints->setObjectName(QString::fromUtf8("QG_CadToolBarPoints"));
        QG_CadToolBarPoints->resize(56, 336);
        QSizePolicy sizePolicy(static_cast<QSizePolicy::Policy>(0), static_cast<QSizePolicy::Policy>(1));
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(QG_CadToolBarPoints->sizePolicy().hasHeightForWidth());
        QG_CadToolBarPoints->setSizePolicy(sizePolicy);
        QG_CadToolBarPoints->setMinimumSize(QSize(56, 336));
        bBack = new QToolButton(QG_CadToolBarPoints);
        bBack->setObjectName(QString::fromUtf8("bBack"));
        bBack->setGeometry(QRect(0, 0, 56, 20));
        bBack->setIcon(qt_get_icon(image0_ID));
        bPoint = new QToolButton(QG_CadToolBarPoints);
        bPoint->setObjectName(QString::fromUtf8("bPoint"));
        bPoint->setGeometry(QRect(0, 20, 28, 28));
        bPoint->setIcon(qt_get_icon(image1_ID));

        retranslateUi(QG_CadToolBarPoints);
        QObject::connect(bPoint, SIGNAL(clicked()), QG_CadToolBarPoints, SLOT(drawPoint()));
        QObject::connect(bBack, SIGNAL(clicked()), QG_CadToolBarPoints, SLOT(back()));

        QMetaObject::connectSlotsByName(QG_CadToolBarPoints);
    } // setupUi

    void retranslateUi(QWidget *QG_CadToolBarPoints)
    {
        QG_CadToolBarPoints->setWindowTitle(QApplication::translate("QG_CadToolBarPoints", "Points", 0, QApplication::UnicodeUTF8));
        bBack->setText(QString());
#ifndef QT_NO_TOOLTIP
        bBack->setProperty("toolTip", QVariant(QApplication::translate("QG_CadToolBarPoints", "Back to main menu", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
        bPoint->setText(QString());
#ifndef QT_NO_TOOLTIP
        bPoint->setProperty("toolTip", QVariant(QApplication::translate("QG_CadToolBarPoints", "Single points", 0, QApplication::UnicodeUTF8)));
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
"18 16 2 1",
". c None",
"# c #000000",
".........#........",
".........#........",
".......#####......",
".........#........",
".........#........",
"..................",
"..................",
"..................",
"..................",
"...............#..",
"...............#..",
"..#..........#####",
"..#............#..",
"#####..........#..",
"..#...............",
"..#..............."};


    switch (id) {
        case image0_ID: return QPixmap((const char**)image0_data);
        case image1_ID: return QPixmap((const char**)image1_data);
        default: return QPixmap();
    } // switch
    } // icon

};

namespace Ui {
    class QG_CadToolBarPoints: public Ui_QG_CadToolBarPoints {};
} // namespace Ui

QT_END_NAMESPACE

class QG_CadToolBarPoints : public QWidget, public Ui::QG_CadToolBarPoints
{
    Q_OBJECT

public:
    QG_CadToolBarPoints(QWidget* parent = 0, const char* name = 0, Qt::WindowFlags fl = 0);
    ~QG_CadToolBarPoints();

public slots:
    virtual void mousePressEvent( QMouseEvent * e );
    virtual void contextMenuEvent( QContextMenuEvent * e );
    virtual void setCadToolBar( QG_CadToolBar * tb );
    virtual void drawPoint();
    virtual void back();

protected:
    QG_ActionHandler* actionHandler;
    QG_CadToolBar* cadToolBar;

protected slots:
    virtual void languageChange();

private:
    void init();

};

#endif // QG_CADTOOLBARPOINTS_H
