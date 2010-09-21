#ifndef QG_CADTOOLBARINFO_H
#define QG_CADTOOLBARINFO_H

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

class Ui_QG_CadToolBarInfo
{
public:
    QToolButton *bDist;
    QToolButton *bDist2;
    QToolButton *bBack;
    QToolButton *bAngle;
    QToolButton *bTotalLength;
    QToolButton *bArea;

    void setupUi(QWidget *QG_CadToolBarInfo)
    {
        if (QG_CadToolBarInfo->objectName().isEmpty())
            QG_CadToolBarInfo->setObjectName(QString::fromUtf8("QG_CadToolBarInfo"));
        QG_CadToolBarInfo->resize(56, 336);
        QSizePolicy sizePolicy(static_cast<QSizePolicy::Policy>(0), static_cast<QSizePolicy::Policy>(1));
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(QG_CadToolBarInfo->sizePolicy().hasHeightForWidth());
        QG_CadToolBarInfo->setSizePolicy(sizePolicy);
        QG_CadToolBarInfo->setMinimumSize(QSize(56, 336));
        bDist = new QToolButton(QG_CadToolBarInfo);
        bDist->setObjectName(QString::fromUtf8("bDist"));
        bDist->setGeometry(QRect(0, 20, 28, 28));
        bDist->setIcon(qt_get_icon(image0_ID));
        bDist2 = new QToolButton(QG_CadToolBarInfo);
        bDist2->setObjectName(QString::fromUtf8("bDist2"));
        bDist2->setGeometry(QRect(28, 20, 28, 28));
        bDist2->setIcon(qt_get_icon(image1_ID));
        bBack = new QToolButton(QG_CadToolBarInfo);
        bBack->setObjectName(QString::fromUtf8("bBack"));
        bBack->setGeometry(QRect(0, 0, 56, 20));
        bBack->setIcon(qt_get_icon(image2_ID));
        bAngle = new QToolButton(QG_CadToolBarInfo);
        bAngle->setObjectName(QString::fromUtf8("bAngle"));
        bAngle->setGeometry(QRect(0, 48, 28, 28));
        bAngle->setIcon(qt_get_icon(image3_ID));
        bTotalLength = new QToolButton(QG_CadToolBarInfo);
        bTotalLength->setObjectName(QString::fromUtf8("bTotalLength"));
        bTotalLength->setGeometry(QRect(28, 48, 28, 28));
        bTotalLength->setIcon(qt_get_icon(image4_ID));
        bArea = new QToolButton(QG_CadToolBarInfo);
        bArea->setObjectName(QString::fromUtf8("bArea"));
        bArea->setGeometry(QRect(0, 76, 28, 28));
        bArea->setIcon(qt_get_icon(image5_ID));

        retranslateUi(QG_CadToolBarInfo);
        QObject::connect(bDist, SIGNAL(clicked()), QG_CadToolBarInfo, SLOT(infoDist()));
        QObject::connect(bBack, SIGNAL(clicked()), QG_CadToolBarInfo, SLOT(back()));
        QObject::connect(bDist2, SIGNAL(clicked()), QG_CadToolBarInfo, SLOT(infoDist2()));
        QObject::connect(bAngle, SIGNAL(clicked()), QG_CadToolBarInfo, SLOT(infoAngle()));
        QObject::connect(bTotalLength, SIGNAL(clicked()), QG_CadToolBarInfo, SLOT(infoTotalLength()));
        QObject::connect(bArea, SIGNAL(clicked()), QG_CadToolBarInfo, SLOT(infoArea()));

        QMetaObject::connectSlotsByName(QG_CadToolBarInfo);
    } // setupUi

    void retranslateUi(QWidget *QG_CadToolBarInfo)
    {
        QG_CadToolBarInfo->setWindowTitle(QApplication::translate("QG_CadToolBarInfo", "Info", 0, QApplication::UnicodeUTF8));
        bDist->setText(QString());
#ifndef QT_NO_TOOLTIP
        bDist->setProperty("toolTip", QVariant(QApplication::translate("QG_CadToolBarInfo", "Distance (Point, Point)", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
        bDist2->setText(QString());
#ifndef QT_NO_TOOLTIP
        bDist2->setProperty("toolTip", QVariant(QApplication::translate("QG_CadToolBarInfo", "Distance (Entity, Point)", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
        bBack->setText(QString());
#ifndef QT_NO_TOOLTIP
        bBack->setProperty("toolTip", QVariant(QApplication::translate("QG_CadToolBarInfo", "Back to main menu", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
        bAngle->setText(QString());
#ifndef QT_NO_TOOLTIP
        bAngle->setProperty("toolTip", QVariant(QApplication::translate("QG_CadToolBarInfo", "Angle", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
        bTotalLength->setText(QString());
#ifndef QT_NO_TOOLTIP
        bTotalLength->setProperty("toolTip", QVariant(QApplication::translate("QG_CadToolBarInfo", "Total length of selected entities", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
        bArea->setText(QString());
#ifndef QT_NO_TOOLTIP
        bArea->setProperty("toolTip", QVariant(QApplication::translate("QG_CadToolBarInfo", "Area of polygon", 0, QApplication::UnicodeUTF8)));
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
"a c #000000",
"# c #ff0000",
"..................",
"..................",
"..................",
"..................",
"###...............",
"#aaaaaa...........",
"##aaaa............",
"...aaaa...........",
"....a..aa.........",
".........aa..a....",
"...........aaaa...",
"............aaaa##",
"...........aaaaaa#",
"...............###",
"..................",
"..................",
"..................",
".................."};


    static const char* const image1_data[] = { 
"18 18 3 1",
". c None",
"a c #000000",
"# c #ff0000",
"......#...........",
".....#............",
".....#............",
"....#.............",
"....#.............",
"...#..............",
"...aaaaaa.........",
"..#.aaaa..........",
"..#..aaaa.........",
".#....a..aa..a....",
".#.........aaaa...",
"#...........aaaa##",
"#..........aaaaaa#",
"...............###",
"..................",
"..................",
"..................",
".................."};


    static const char* const image2_data[] = { 
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


    static const char* const image3_data[] = { 
"20 20 3 1",
". c None",
"# c #000000",
"a c #ff0000",
"....#....#..........",
"######...#..........",
"...####.#...........",
"...######...........",
".......#.aa.........",
".......#...a........",
"......#.....a.......",
"......#......a......",
".....#........a.....",
".....#.........a....",
"....#..........a..##",
"....#...........##..",
"...#..........###...",
"...#........##..##..",
"..#.......##....###.",
"..#.....##......####",
".#....##........###.",
".#..##............#.",
"#.##..............#.",
"##................#."};


    static const char* const image4_data[] = { 
"18 18 3 1",
". c None",
"a c #000000",
"# c #ff0000",
"..................",
"..................",
"..###############.",
"..................",
"........##........",
"......##..........",
"....##............",
"..##..............",
"..................",
"....##............",
"...#..#...aaaaaa..",
"..#....#...a......",
"..#....#....a.....",
"...#..#......a....",
"....##......a.....",
"...........a......",
"..........aaaaaa..",
".................."};


    static const char* const image5_data[] = { 
"20 20 3 1",
". c None",
"# c #000000",
"a c #ff0000",
"....................",
"....................",
"....................",
".........#########..",
"........#aaaaaaaa#..",
".......#aaaaaaaaa#..",
"......#aaaaaaaaaa#..",
".....#aaaaaaaaaaa#..",
"....#aaaaaaaaaaaa#..",
"...#aaaaaaaaaaaaa#..",
"..#aaaaaaaaaaaaaa#..",
".#aaaaaaaaaaaaaaa#..",
".#aaaaaaaaaaaaaaa#..",
".#aaaaaaaaaaaaaaa#..",
".#aaaaaaaaaaaaaaa#..",
".#################..",
"....................",
"....................",
"....................",
"...................."};


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
    class QG_CadToolBarInfo: public Ui_QG_CadToolBarInfo {};
} // namespace Ui

QT_END_NAMESPACE

class QG_CadToolBarInfo : public QWidget, public Ui::QG_CadToolBarInfo
{
    Q_OBJECT

public:
    QG_CadToolBarInfo(QWidget* parent = 0, const char* name = 0, Qt::WindowFlags fl = 0);
    ~QG_CadToolBarInfo();

public slots:
    virtual void mousePressEvent( QMouseEvent * e );
    virtual void contextMenuEvent( QContextMenuEvent * e );
    virtual void setCadToolBar( QG_CadToolBar * tb );
    virtual void infoDist();
    virtual void infoDist2();
    virtual void infoAngle();
    virtual void infoTotalLength();
    virtual void infoArea();
    virtual void back();

protected:
    QG_ActionHandler* actionHandler;
    QG_CadToolBar* cadToolBar;

protected slots:
    virtual void languageChange();

private:
    void init();

};

#endif // QG_CADTOOLBARINFO_H
