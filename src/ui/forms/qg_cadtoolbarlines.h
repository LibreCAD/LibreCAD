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
#ifndef QG_CADTOOLBARLINES_H
#define QG_CADTOOLBARLINES_H

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

class Ui_QG_CadToolBarLines
{
public:
    QToolButton *bBack;
    QToolButton *bNormal;
    QToolButton *bAngle;
    QToolButton *bHorizontal;
    QToolButton *bVertical;
    QToolButton *bRectangle;
    QToolButton *bBisector;
    QToolButton *bParallel;
    QToolButton *bTangent1;
    QToolButton *bTangent2;
    QToolButton *bOrthogonal;
    QToolButton *bRelAngle;
    QToolButton *bPolygon;
    QToolButton *bPolygon2;
    QToolButton *bFree;
    QToolButton *bParallelThrough;

    void setupUi(QWidget *QG_CadToolBarLines)
    {
        if (QG_CadToolBarLines->objectName().isEmpty())
            QG_CadToolBarLines->setObjectName(QString::fromUtf8("QG_CadToolBarLines"));
        QG_CadToolBarLines->resize(56, 338);
        QSizePolicy sizePolicy(static_cast<QSizePolicy::Policy>(0), static_cast<QSizePolicy::Policy>(1));
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(QG_CadToolBarLines->sizePolicy().hasHeightForWidth());
        QG_CadToolBarLines->setSizePolicy(sizePolicy);
        QG_CadToolBarLines->setMinimumSize(QSize(56, 336));
        bBack = new QToolButton(QG_CadToolBarLines);
        bBack->setObjectName(QString::fromUtf8("bBack"));
        bBack->setGeometry(QRect(0, 0, 56, 20));
        bBack->setIcon(qt_get_icon(image0_ID));
        bNormal = new QToolButton(QG_CadToolBarLines);
        bNormal->setObjectName(QString::fromUtf8("bNormal"));
        bNormal->setGeometry(QRect(0, 20, 28, 28));
        bNormal->setIcon(qt_get_icon(image1_ID));
        bAngle = new QToolButton(QG_CadToolBarLines);
        bAngle->setObjectName(QString::fromUtf8("bAngle"));
        bAngle->setGeometry(QRect(28, 20, 28, 28));
        bAngle->setIcon(qt_get_icon(image2_ID));
        bHorizontal = new QToolButton(QG_CadToolBarLines);
        bHorizontal->setObjectName(QString::fromUtf8("bHorizontal"));
        bHorizontal->setGeometry(QRect(0, 48, 28, 28));
        bHorizontal->setIcon(qt_get_icon(image3_ID));
        bVertical = new QToolButton(QG_CadToolBarLines);
        bVertical->setObjectName(QString::fromUtf8("bVertical"));
        bVertical->setGeometry(QRect(28, 48, 28, 28));
        bVertical->setIcon(qt_get_icon(image4_ID));
        bRectangle = new QToolButton(QG_CadToolBarLines);
        bRectangle->setObjectName(QString::fromUtf8("bRectangle"));
        bRectangle->setGeometry(QRect(0, 76, 28, 28));
        bRectangle->setIcon(qt_get_icon(image5_ID));
        bBisector = new QToolButton(QG_CadToolBarLines);
        bBisector->setObjectName(QString::fromUtf8("bBisector"));
        bBisector->setGeometry(QRect(28, 76, 28, 28));
        bBisector->setIcon(qt_get_icon(image6_ID));
        bParallel = new QToolButton(QG_CadToolBarLines);
        bParallel->setObjectName(QString::fromUtf8("bParallel"));
        bParallel->setGeometry(QRect(0, 104, 28, 28));
        bParallel->setIcon(qt_get_icon(image7_ID));
        bTangent1 = new QToolButton(QG_CadToolBarLines);
        bTangent1->setObjectName(QString::fromUtf8("bTangent1"));
        bTangent1->setGeometry(QRect(0, 132, 28, 28));
        bTangent1->setIcon(qt_get_icon(image8_ID));
        bTangent2 = new QToolButton(QG_CadToolBarLines);
        bTangent2->setObjectName(QString::fromUtf8("bTangent2"));
        bTangent2->setGeometry(QRect(28, 132, 28, 28));
        bTangent2->setIcon(qt_get_icon(image9_ID));
        bOrthogonal = new QToolButton(QG_CadToolBarLines);
        bOrthogonal->setObjectName(QString::fromUtf8("bOrthogonal"));
        bOrthogonal->setGeometry(QRect(0, 160, 28, 28));
        bOrthogonal->setIcon(qt_get_icon(image10_ID));
        bRelAngle = new QToolButton(QG_CadToolBarLines);
        bRelAngle->setObjectName(QString::fromUtf8("bRelAngle"));
        bRelAngle->setGeometry(QRect(28, 160, 28, 28));
        bRelAngle->setIcon(qt_get_icon(image11_ID));
        bPolygon = new QToolButton(QG_CadToolBarLines);
        bPolygon->setObjectName(QString::fromUtf8("bPolygon"));
        bPolygon->setGeometry(QRect(0, 188, 28, 28));
        bPolygon->setIcon(qt_get_icon(image12_ID));
        bPolygon2 = new QToolButton(QG_CadToolBarLines);
        bPolygon2->setObjectName(QString::fromUtf8("bPolygon2"));
        bPolygon2->setGeometry(QRect(28, 188, 28, 28));
        bPolygon2->setIcon(qt_get_icon(image13_ID));
        bFree = new QToolButton(QG_CadToolBarLines);
        bFree->setObjectName(QString::fromUtf8("bFree"));
        bFree->setGeometry(QRect(0, 216, 28, 28));
        bFree->setIcon(qt_get_icon(image14_ID));
        bParallelThrough = new QToolButton(QG_CadToolBarLines);
        bParallelThrough->setObjectName(QString::fromUtf8("bParallelThrough"));
        bParallelThrough->setGeometry(QRect(28, 104, 28, 28));
        bParallelThrough->setIcon(qt_get_icon(image15_ID));

        retranslateUi(QG_CadToolBarLines);
        QObject::connect(bNormal, SIGNAL(clicked()), QG_CadToolBarLines, SLOT(drawLine()));
        QObject::connect(bFree, SIGNAL(clicked()), QG_CadToolBarLines, SLOT(drawLineFree()));
        QObject::connect(bParallel, SIGNAL(clicked()), QG_CadToolBarLines, SLOT(drawLineParallel()));
        QObject::connect(bAngle, SIGNAL(clicked()), QG_CadToolBarLines, SLOT(drawLineAngle()));
        QObject::connect(bHorizontal, SIGNAL(clicked()), QG_CadToolBarLines, SLOT(drawLineHorizontal()));
        QObject::connect(bBisector, SIGNAL(clicked()), QG_CadToolBarLines, SLOT(drawLineBisector()));
        QObject::connect(bTangent1, SIGNAL(clicked()), QG_CadToolBarLines, SLOT(drawLineTangent1()));
        QObject::connect(bTangent2, SIGNAL(clicked()), QG_CadToolBarLines, SLOT(drawLineTangent2()));
        QObject::connect(bRectangle, SIGNAL(clicked()), QG_CadToolBarLines, SLOT(drawLineRectangle()));
        QObject::connect(bBack, SIGNAL(clicked()), QG_CadToolBarLines, SLOT(back()));
        QObject::connect(bRelAngle, SIGNAL(clicked()), QG_CadToolBarLines, SLOT(drawLineRelAngle()));
        QObject::connect(bVertical, SIGNAL(clicked()), QG_CadToolBarLines, SLOT(drawLineVertical()));
        QObject::connect(bOrthogonal, SIGNAL(clicked()), QG_CadToolBarLines, SLOT(drawLineOrthogonal()));
        QObject::connect(bPolygon, SIGNAL(clicked()), QG_CadToolBarLines, SLOT(drawLinePolygon()));
        QObject::connect(bPolygon2, SIGNAL(clicked()), QG_CadToolBarLines, SLOT(drawLinePolygon2()));
        QObject::connect(bParallelThrough, SIGNAL(clicked()), QG_CadToolBarLines, SLOT(drawLineParallelThrough()));

        QMetaObject::connectSlotsByName(QG_CadToolBarLines);
    } // setupUi

    void retranslateUi(QWidget *QG_CadToolBarLines)
    {
        QG_CadToolBarLines->setWindowTitle(QApplication::translate("QG_CadToolBarLines", "Lines", 0, QApplication::UnicodeUTF8));
        bBack->setText(QString());
#ifndef QT_NO_TOOLTIP
        bBack->setProperty("toolTip", QVariant(QApplication::translate("QG_CadToolBarLines", "Back to main menu", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
        bNormal->setText(QString());
#ifndef QT_NO_TOOLTIP
        bNormal->setProperty("toolTip", QVariant(QApplication::translate("QG_CadToolBarLines", "Line with two points", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
        bAngle->setText(QString());
#ifndef QT_NO_TOOLTIP
        bAngle->setProperty("toolTip", QVariant(QApplication::translate("QG_CadToolBarLines", "Line with given angle", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
        bHorizontal->setText(QString());
#ifndef QT_NO_TOOLTIP
        bHorizontal->setProperty("toolTip", QVariant(QApplication::translate("QG_CadToolBarLines", "Horizontal lines", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
        bVertical->setText(QString());
#ifndef QT_NO_TOOLTIP
        bVertical->setProperty("toolTip", QVariant(QApplication::translate("QG_CadToolBarLines", "Vertical lines", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
        bRectangle->setText(QString());
#ifndef QT_NO_TOOLTIP
        bRectangle->setProperty("toolTip", QVariant(QApplication::translate("QG_CadToolBarLines", "Rectangles", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
        bBisector->setText(QString());
#ifndef QT_NO_TOOLTIP
        bBisector->setProperty("toolTip", QVariant(QApplication::translate("QG_CadToolBarLines", "Bisectors", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
        bParallel->setText(QString());
#ifndef QT_NO_TOOLTIP
        bParallel->setProperty("toolTip", QVariant(QApplication::translate("QG_CadToolBarLines", "Parallels with distance", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
        bTangent1->setText(QString());
#ifndef QT_NO_TOOLTIP
        bTangent1->setProperty("toolTip", QVariant(QApplication::translate("QG_CadToolBarLines", "Tangents from point to circle", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
        bTangent2->setText(QString());
#ifndef QT_NO_TOOLTIP
        bTangent2->setProperty("toolTip", QVariant(QApplication::translate("QG_CadToolBarLines", "Tangents from circle to circle", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
        bOrthogonal->setText(QString());
#ifndef QT_NO_TOOLTIP
        bOrthogonal->setProperty("toolTip", QVariant(QApplication::translate("QG_CadToolBarLines", "Orthogonal lines", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
        bRelAngle->setText(QString());
#ifndef QT_NO_TOOLTIP
        bRelAngle->setProperty("toolTip", QVariant(QApplication::translate("QG_CadToolBarLines", "Lines with relative angles", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
        bPolygon->setText(QString());
#ifndef QT_NO_TOOLTIP
        bPolygon->setProperty("toolTip", QVariant(QApplication::translate("QG_CadToolBarLines", "Polygons with Center and Corner", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
        bPolygon2->setText(QString());
#ifndef QT_NO_TOOLTIP
        bPolygon2->setProperty("toolTip", QVariant(QApplication::translate("QG_CadToolBarLines", "Polygons with two Corners", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
        bFree->setText(QString());
#ifndef QT_NO_TOOLTIP
        bFree->setProperty("toolTip", QVariant(QApplication::translate("QG_CadToolBarLines", "Freehand lines", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
        bParallelThrough->setText(QString());
#ifndef QT_NO_TOOLTIP
        bParallelThrough->setProperty("toolTip", QVariant(QApplication::translate("QG_CadToolBarLines", "Parallels through point", 0, QApplication::UnicodeUTF8)));
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
        image12_ID,
        image13_ID,
        image14_ID,
        image15_ID,
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
"18 13 3 1",
". c None",
"a c #000000",
"# c #ff0000",
"................#.",
"...............###",
"..............a.#.",
".............a....",
"...........aa.....",
"..........a.......",
"........aa........",
".......a..........",
"......a...........",
"....aa............",
".#.a..............",
"###...............",
".#................"};


    static const char* const image2_data[] = { 
"17 11 3 1",
". c None",
"# c #000000",
"a c #ff0000",
"...............##",
"..............#..",
"............##...",
"...........#.....",
".........##......",
"........#........",
"......##.........",
".....#...........",
".a.##............",
"aaa..............",
".a..............."};


    static const char* const image3_data[] = { 
"17 3 3 1",
". c None",
"a c #000000",
"# c #ff0000",
".#...............",
"###aaaaaaaaaaaaaa",
".#..............."};


    static const char* const image4_data[] = { 
"3 17 3 1",
". c None",
"# c #000000",
"a c #ff0000",
".#.",
".#.",
".#.",
".#.",
".#.",
".#.",
".#.",
".#.",
".#.",
".#.",
".#.",
".#.",
".#.",
".#.",
".a.",
"aaa",
".a."};


    static const char* const image5_data[] = { 
"18 15 3 1",
". c None",
"a c #000000",
"# c #ff0000",
".#................",
"###aaaaaaaaaaaaaa.",
".#..............a.",
".a..............a.",
".a..............a.",
".a..............a.",
".a..............a.",
".a..............a.",
".a..............a.",
".a..............a.",
".a..............a.",
".a..............a.",
".a..............#.",
".aaaaaaaaaaaaaa###",
"................#."};


    static const char* const image6_data[] = { 
"18 18 4 1",
". c None",
"b c #000000",
"a c #0000ff",
"# c #ff0000",
"......#...........",
"......#...........",
".....#............",
".....#..aaaa......",
".....#............",
"....#...aaaa......",
"....#.........b...",
"....#a.......b....",
"....#.aa...bb.....",
"...#....a.b.......",
"...#.....a........",
"...#...bba....aaaa",
"..#...b...a.......",
"..#..b....a...aaaa",
"..#bb.....a.......",
".#b........a......",
"#b#########a######",
".#................"};


    static const char* const image7_data[] = { 
"17 15 4 1",
". c None",
"b c #000000",
"a c #0000ff",
"# c #ff0000",
"............##...",
"..........##.....",
"........##.......",
"......##.........",
"....##...........",
"..##.a...........",
"##...a...........",
"......a........bb",
"......a......bb..",
".......a...bb....",
".......a.bb......",
".......bb........",
".....bb..........",
"...bb............",
"................."};


    static const char* const image8_data[] = { 
"18 18 3 1",
". c None",
"a c #000000",
"# c #ff0000",
"................#.",
"...............###",
".............aa.#.",
"..........aaa.....",
".......aaa........",
".....aa...........",
"...aa####.........",
"..#......#........",
".#........#.......",
"#..........#......",
"#..........#......",
"#..........#......",
"#..........#......",
"#..........#......",
"#..........#......",
".#........#.......",
"..#......#........",
"...######........."};


    static const char* const image9_data[] = { 
"18 18 3 1",
". c None",
"a c #000000",
"# c #ff0000",
"............####..",
"...........a....#.",
".........aa......#",
"........a.#......#",
"......aa..#......#",
".....a....#......#",
"...aa####..#....#.",
"..a......#..####..",
".#........#.......",
"#..........#......",
"#..........#......",
"#..........#......",
"#..........#......",
"#..........#......",
"#..........#......",
".#........#.......",
"..#......#........",
"...######........."};


    static const char* const image10_data[] = { 
"18 13 3 1",
". c None",
"# c #000000",
"a c #ff0000",
"....#............#",
".....#.........##.",
".....#........#...",
"......#......#....",
".......#...##.....",
"........a.#.......",
".......aaa........",
".......#a#........",
"......#...#.......",
"....##.....#......",
"...#.......#......",
"..#.........#.....",
"##...........#...."};


    static const char* const image11_data[] = { 
"18 13 3 1",
". c None",
"# c #000000",
"a c #ff0000",
"..........#......#",
"..........#....##.",
"..........#...#...",
".........#...#....",
".........#.##.....",
"........a##.......",
".......aaa........",
".......#a.........",
"......#.#.........",
"....##.#..........",
"...#...#..........",
"..#....#..........",
"##....#..........."};


    static const char* const image12_data[] = { 
"18 18 3 1",
". c None",
"# c #000000",
"a c #ff0000",
"........##........",
".......#..#.......",
".....##....#......",
"....#.......##....",
"...#..........#...",
".##............##.",
"#................#",
"#................#",
".#......aa......#.",
".#......aa......#.",
".#..............#.",
"..#.............#.",
"..#............#..",
"..#............#..",
"..#............#..",
"...#..........a...",
"...##########aaa..",
"..............a..."};


    static const char* const image13_data[] = { 
"18 18 3 1",
". c None",
"# c #000000",
"a c #ff0000",
"........##........",
".......#..#.......",
".....##....#......",
"....#.......##....",
"...#..........#...",
".##............##.",
"#................#",
"#................#",
".#..............#.",
".#..............#.",
".#..............#.",
"..#.............#.",
"..#............#..",
"..#............#..",
"..#............#..",
"...a..........a...",
"..aaa########aaa..",
"...a..........a..."};


    static const char* const image14_data[] = { 
"18 13 2 1",
". c None",
"# c #000000",
"..................",
".............###..",
".....##.....#...#.",
"....#..#...#......",
"...#...#...#......",
"...#...#...#......",
"..#....#...#......",
"..#...#.....#.....",
".#....#.....#.....",
".#....#.....#.....",
".#.....#...#......",
"........###.......",
".................."};


    static const char* const image15_data[] = { 
"17 15 3 1",
". c None",
"a c #000000",
"# c #ff0000",
"............##...",
"..........##.....",
"........##.......",
"......##.........",
"....##...........",
"..##.............",
"##...............",
"...............aa",
".............aa..",
".........#.aa....",
"........###......",
".......aa#.......",
".....aa..........",
"...aa............",
"................."};


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
        case image12_ID: return QPixmap((const char**)image12_data);
        case image13_ID: return QPixmap((const char**)image13_data);
        case image14_ID: return QPixmap((const char**)image14_data);
        case image15_ID: return QPixmap((const char**)image15_data);
        default: return QPixmap();
    } // switch
    } // icon

};

namespace Ui {
    class QG_CadToolBarLines: public Ui_QG_CadToolBarLines {};
} // namespace Ui

QT_END_NAMESPACE

class QG_CadToolBarLines : public QWidget, public Ui::QG_CadToolBarLines
{
    Q_OBJECT

public:
    QG_CadToolBarLines(QWidget* parent = 0, const char* name = 0, Qt::WindowFlags fl = 0);
    ~QG_CadToolBarLines();

public slots:
    virtual void contextMenuEvent( QContextMenuEvent * e );
    virtual void setCadToolBar( QG_CadToolBar * tb );
    virtual void drawLine();
    virtual void drawLineAngle();
    virtual void drawLineHorizontal();
    virtual void drawLineHorVert();
    virtual void drawLineVertical();
    virtual void drawLineParallel();
    virtual void drawLineParallelThrough();
    virtual void drawLineRectangle();
    virtual void drawLineBisector();
    virtual void drawLineTangent1();
    virtual void drawLineTangent2();
    virtual void drawLineOrthogonal();
    virtual void drawLineRelAngle();
    virtual void drawLineFree();
    virtual void drawLinePolygon();
    virtual void drawLinePolygon2();
    virtual void back();

protected:
    QG_CadToolBar* cadToolBar;
    QG_ActionHandler* actionHandler;

protected slots:
    virtual void languageChange();

private:
    void init();

};

#endif // QG_CADTOOLBARLINES_H
