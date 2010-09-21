#ifndef QG_CADTOOLBARSNAP_H
#define QG_CADTOOLBARSNAP_H

#include <qvariant.h>

class QG_CadToolBar;
class QG_ActionHandler;

#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QHeaderView>
#include <QtGui/QToolButton>
#include <QtGui/QWidget>
#include "qg_actionhandler.h"

QT_BEGIN_NAMESPACE

class Ui_QG_CadToolBarSnap
{
public:
    QToolButton *bBack;
    QToolButton *bGrid;
    QToolButton *bFree;
    QToolButton *bEndpoint;
    QToolButton *bOnEntity;
    QToolButton *bCenter;
    QToolButton *bMiddle;
    QToolButton *bDist;
    QToolButton *bIntersection;
    QToolButton *bResNothing;
    QToolButton *bResOrthogonal;
    QToolButton *bResHorizontal;
    QToolButton *bResVertical;
    QToolButton *bRelZero;
    QToolButton *bLockRelZero;
    QToolButton *bIntersectionManual;

    void setupUi(QWidget *QG_CadToolBarSnap)
    {
        if (QG_CadToolBarSnap->objectName().isEmpty())
            QG_CadToolBarSnap->setObjectName(QString::fromUtf8("QG_CadToolBarSnap"));
        QG_CadToolBarSnap->resize(56, 336);
        QSizePolicy sizePolicy(static_cast<QSizePolicy::Policy>(0), static_cast<QSizePolicy::Policy>(1));
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(QG_CadToolBarSnap->sizePolicy().hasHeightForWidth());
        QG_CadToolBarSnap->setSizePolicy(sizePolicy);
        QG_CadToolBarSnap->setMinimumSize(QSize(56, 336));
        bBack = new QToolButton(QG_CadToolBarSnap);
        bBack->setObjectName(QString::fromUtf8("bBack"));
        bBack->setGeometry(QRect(0, 0, 56, 20));
        bBack->setIcon(qt_get_icon(image0_ID));
        bGrid = new QToolButton(QG_CadToolBarSnap);
        bGrid->setObjectName(QString::fromUtf8("bGrid"));
        bGrid->setGeometry(QRect(28, 20, 28, 28));
        bGrid->setIcon(qt_get_icon(image1_ID));
        bGrid->setCheckable(true);
        bFree = new QToolButton(QG_CadToolBarSnap);
        bFree->setObjectName(QString::fromUtf8("bFree"));
        bFree->setGeometry(QRect(0, 20, 28, 28));
        bFree->setIcon(qt_get_icon(image2_ID));
        bFree->setCheckable(true);
        bEndpoint = new QToolButton(QG_CadToolBarSnap);
        bEndpoint->setObjectName(QString::fromUtf8("bEndpoint"));
        bEndpoint->setGeometry(QRect(0, 48, 28, 28));
        bEndpoint->setIcon(qt_get_icon(image3_ID));
        bEndpoint->setCheckable(true);
        bOnEntity = new QToolButton(QG_CadToolBarSnap);
        bOnEntity->setObjectName(QString::fromUtf8("bOnEntity"));
        bOnEntity->setGeometry(QRect(28, 48, 28, 28));
        bOnEntity->setIcon(qt_get_icon(image4_ID));
        bOnEntity->setCheckable(true);
        bCenter = new QToolButton(QG_CadToolBarSnap);
        bCenter->setObjectName(QString::fromUtf8("bCenter"));
        bCenter->setGeometry(QRect(0, 76, 28, 28));
        bCenter->setIcon(qt_get_icon(image5_ID));
        bCenter->setCheckable(true);
        bMiddle = new QToolButton(QG_CadToolBarSnap);
        bMiddle->setObjectName(QString::fromUtf8("bMiddle"));
        bMiddle->setGeometry(QRect(28, 76, 28, 28));
        bMiddle->setIcon(qt_get_icon(image6_ID));
        bMiddle->setCheckable(true);
        bDist = new QToolButton(QG_CadToolBarSnap);
        bDist->setObjectName(QString::fromUtf8("bDist"));
        bDist->setGeometry(QRect(0, 104, 28, 28));
        bDist->setIcon(qt_get_icon(image7_ID));
        bDist->setCheckable(true);
        bIntersection = new QToolButton(QG_CadToolBarSnap);
        bIntersection->setObjectName(QString::fromUtf8("bIntersection"));
        bIntersection->setGeometry(QRect(28, 104, 28, 28));
        bIntersection->setIcon(qt_get_icon(image8_ID));
        bIntersection->setCheckable(true);
        bResNothing = new QToolButton(QG_CadToolBarSnap);
        bResNothing->setObjectName(QString::fromUtf8("bResNothing"));
        bResNothing->setGeometry(QRect(0, 180, 28, 28));
        bResNothing->setIcon(qt_get_icon(image2_ID));
        bResNothing->setCheckable(true);
        bResOrthogonal = new QToolButton(QG_CadToolBarSnap);
        bResOrthogonal->setObjectName(QString::fromUtf8("bResOrthogonal"));
        bResOrthogonal->setGeometry(QRect(28, 180, 28, 28));
        bResOrthogonal->setIcon(qt_get_icon(image9_ID));
        bResOrthogonal->setCheckable(true);
        bResHorizontal = new QToolButton(QG_CadToolBarSnap);
        bResHorizontal->setObjectName(QString::fromUtf8("bResHorizontal"));
        bResHorizontal->setGeometry(QRect(0, 208, 28, 28));
        bResHorizontal->setIcon(qt_get_icon(image10_ID));
        bResHorizontal->setCheckable(true);
        bResVertical = new QToolButton(QG_CadToolBarSnap);
        bResVertical->setObjectName(QString::fromUtf8("bResVertical"));
        bResVertical->setGeometry(QRect(28, 208, 28, 28));
        bResVertical->setIcon(qt_get_icon(image11_ID));
        bResVertical->setCheckable(true);
        bRelZero = new QToolButton(QG_CadToolBarSnap);
        bRelZero->setObjectName(QString::fromUtf8("bRelZero"));
        bRelZero->setGeometry(QRect(0, 260, 28, 28));
        bRelZero->setIcon(qt_get_icon(image12_ID));
        bRelZero->setCheckable(false);
        bLockRelZero = new QToolButton(QG_CadToolBarSnap);
        bLockRelZero->setObjectName(QString::fromUtf8("bLockRelZero"));
        bLockRelZero->setGeometry(QRect(28, 260, 28, 28));
        bLockRelZero->setIcon(qt_get_icon(image13_ID));
        bLockRelZero->setCheckable(true);
        bIntersectionManual = new QToolButton(QG_CadToolBarSnap);
        bIntersectionManual->setObjectName(QString::fromUtf8("bIntersectionManual"));
        bIntersectionManual->setGeometry(QRect(0, 132, 28, 28));
        bIntersectionManual->setIcon(qt_get_icon(image14_ID));
        bIntersectionManual->setCheckable(false);

        retranslateUi(QG_CadToolBarSnap);
        QObject::connect(bFree, SIGNAL(clicked()), QG_CadToolBarSnap, SLOT(snapFree()));
        QObject::connect(bGrid, SIGNAL(clicked()), QG_CadToolBarSnap, SLOT(snapGrid()));
        QObject::connect(bEndpoint, SIGNAL(clicked()), QG_CadToolBarSnap, SLOT(snapEndpoint()));
        QObject::connect(bOnEntity, SIGNAL(clicked()), QG_CadToolBarSnap, SLOT(snapOnEntity()));
        QObject::connect(bCenter, SIGNAL(clicked()), QG_CadToolBarSnap, SLOT(snapCenter()));
        QObject::connect(bMiddle, SIGNAL(clicked()), QG_CadToolBarSnap, SLOT(snapMiddle()));
        QObject::connect(bDist, SIGNAL(clicked()), QG_CadToolBarSnap, SLOT(snapDist()));
        QObject::connect(bIntersection, SIGNAL(clicked()), QG_CadToolBarSnap, SLOT(snapIntersection()));
        QObject::connect(bBack, SIGNAL(clicked()), QG_CadToolBarSnap, SLOT(back()));
        QObject::connect(bResOrthogonal, SIGNAL(clicked()), QG_CadToolBarSnap, SLOT(restrictOrthogonal()));
        QObject::connect(bResNothing, SIGNAL(clicked()), QG_CadToolBarSnap, SLOT(restrictNothing()));
        QObject::connect(bRelZero, SIGNAL(clicked()), QG_CadToolBarSnap, SLOT(setRelativeZero()));
        QObject::connect(bLockRelZero, SIGNAL(toggled(bool)), QG_CadToolBarSnap, SLOT(lockRelativeZero(bool)));
        QObject::connect(bResHorizontal, SIGNAL(clicked()), QG_CadToolBarSnap, SLOT(restrictHorizontal()));
        QObject::connect(bResVertical, SIGNAL(clicked()), QG_CadToolBarSnap, SLOT(restrictVertical()));
        QObject::connect(bIntersectionManual, SIGNAL(clicked()), QG_CadToolBarSnap, SLOT(snapIntersectionManual()));

        QMetaObject::connectSlotsByName(QG_CadToolBarSnap);
    } // setupUi

    void retranslateUi(QWidget *QG_CadToolBarSnap)
    {
        QG_CadToolBarSnap->setWindowTitle(QApplication::translate("QG_CadToolBarSnap", "Snap", 0, QApplication::UnicodeUTF8));
        bBack->setText(QString());
#ifndef QT_NO_TOOLTIP
        bBack->setProperty("toolTip", QVariant(QApplication::translate("QG_CadToolBarSnap", "Back to main menu", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
        bGrid->setText(QString());
#ifndef QT_NO_TOOLTIP
        bGrid->setProperty("toolTip", QVariant(QApplication::translate("QG_CadToolBarSnap", "Snap to grid", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
        bFree->setText(QString());
#ifndef QT_NO_TOOLTIP
        bFree->setProperty("toolTip", QVariant(QApplication::translate("QG_CadToolBarSnap", "Free positioning", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
        bEndpoint->setText(QString());
#ifndef QT_NO_TOOLTIP
        bEndpoint->setProperty("toolTip", QVariant(QApplication::translate("QG_CadToolBarSnap", "Snap to Endpoints", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
        bOnEntity->setText(QString());
#ifndef QT_NO_TOOLTIP
        bOnEntity->setProperty("toolTip", QVariant(QApplication::translate("QG_CadToolBarSnap", "Snap to closest point on entity", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
        bCenter->setText(QString());
#ifndef QT_NO_TOOLTIP
        bCenter->setProperty("toolTip", QVariant(QApplication::translate("QG_CadToolBarSnap", "Snap to center points", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
        bMiddle->setText(QString());
#ifndef QT_NO_TOOLTIP
        bMiddle->setProperty("toolTip", QVariant(QApplication::translate("QG_CadToolBarSnap", "Snap to middle points", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
        bDist->setText(QString());
#ifndef QT_NO_TOOLTIP
        bDist->setProperty("toolTip", QVariant(QApplication::translate("QG_CadToolBarSnap", "Snap to point with given distance to endpoint", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
        bIntersection->setText(QString());
#ifndef QT_NO_TOOLTIP
        bIntersection->setProperty("toolTip", QVariant(QApplication::translate("QG_CadToolBarSnap", "Snap to intersections automatically", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
        bResNothing->setText(QString());
#ifndef QT_NO_TOOLTIP
        bResNothing->setProperty("toolTip", QVariant(QApplication::translate("QG_CadToolBarSnap", "No Restriction", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
        bResOrthogonal->setText(QString());
#ifndef QT_NO_TOOLTIP
        bResOrthogonal->setProperty("toolTip", QVariant(QApplication::translate("QG_CadToolBarSnap", "Orthogonal Restriction", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
        bResHorizontal->setText(QString());
#ifndef QT_NO_TOOLTIP
        bResHorizontal->setProperty("toolTip", QVariant(QApplication::translate("QG_CadToolBarSnap", "Horizontal Restriction", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
        bResVertical->setText(QString());
#ifndef QT_NO_TOOLTIP
        bResVertical->setProperty("toolTip", QVariant(QApplication::translate("QG_CadToolBarSnap", "Vertical Restriction", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
        bRelZero->setText(QString());
#ifndef QT_NO_TOOLTIP
        bRelZero->setProperty("toolTip", QVariant(QApplication::translate("QG_CadToolBarSnap", "Move relative Zero", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
        bLockRelZero->setText(QString());
#ifndef QT_NO_TOOLTIP
        bLockRelZero->setProperty("toolTip", QVariant(QApplication::translate("QG_CadToolBarSnap", "Lock relative Zero", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
        bIntersectionManual->setText(QString());
#ifndef QT_NO_TOOLTIP
        bIntersectionManual->setProperty("toolTip", QVariant(QApplication::translate("QG_CadToolBarSnap", "Snap to intersections manually", 0, QApplication::UnicodeUTF8)));
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
"17 17 2 1",
"# c None",
". c #ff0000",
"..###..###..###..",
"..###..###..###..",
"#################",
"#################",
"#################",
"..###..###..###..",
"..###..###..###..",
"#################",
"#################",
"#################",
"..###..###..###..",
"..###..###..###..",
"#################",
"#################",
"#################",
"..###..###..###..",
"..###..###..###.."};


    static const char* const image2_data[] = { 
"5 5 2 1",
". c None",
"# c #000000",
"..#..",
"..#..",
"#####",
"..#..",
"..#.."};


    static const char* const image3_data[] = { 
"18 18 3 1",
". c None",
"a c #000000",
"# c #ff0000",
"......###.........",
"......###.........",
"......###.........",
".......a..........",
".......a..........",
".......a..........",
"........a.........",
"###......a........",
"###.......a.......",
"###a.......aaa.###",
"....aa........a###",
"......aa.......###",
"........aa........",
"..........aa......",
"............aa....",
"..............a###",
"...............###",
"...............###"};


    static const char* const image4_data[] = { 
"18 18 3 1",
". c None",
"a c #000000",
"# c #ff0000",
"..................",
"............###...",
"............###...",
"........###a###...",
"........###.......",
"....###a###.....##",
"....###.....###a##",
"###a###....a###.##",
"###.....###.###...",
"###.....###.......",
"........###.......",
".......a..........",
"......###.........",
"......###.........",
"......###.........",
"......a...........",
".....###..........",
".....###.........."};


    static const char* const image5_data[] = { 
"18 18 3 1",
". c None",
"# c #000000",
"a c #ff0000",
"...............##.",
".............##...",
"...........##.....",
".......aaa#.......",
".......aaa........",
".....##aaa........",
"...##........####.",
".##.......###.....",
"#........#........",
"........#.........",
".......#..........",
"......#...........",
"......#...........",
"......#...........",
".....#............",
".....#........aaa.",
".....#........aaa.",
".....#........aaa."};


    static const char* const image6_data[] = { 
"17 18 3 1",
". c None",
"# c #000000",
"a c #ff0000",
"...............##",
".............##..",
"...........##....",
".......aaa#......",
".......aaa.......",
".....##aaa.......",
"...##........####",
".##.......###....",
"#......aaa.......",
".......aaa.......",
".......aaa.......",
"......#..........",
"......#..........",
"......#..........",
".....#...........",
".....#...........",
".....#...........",
".....#..........."};


    static const char* const image7_data[] = { 
"18 18 3 1",
". c None",
"# c #000000",
"a c #ff0000",
".................#",
"...###.........##.",
"...##........##...",
".##.#......##.....",
"#.....aaa##.......",
"......aaa.........",
".....#aaa......###",
"...##.......###...",
"..#........#......",
"..........#.......",
".......aaa........",
".......aaa........",
".......aaa..#.....",
"........#..##.....",
".......#..##.#....",
".......#...#......",
".......#...#......",
".......#...#......"};


    static const char* const image8_data[] = { 
"18 18 3 1",
". c None",
"# c #000000",
"a c #ff0000",
".............#....",
".............#....",
".............#....",
".............#....",
"............#.....",
"............#.....",
"............#.....",
"#...........#.....",
".####......#......",
".....####..a......",
".........#aaa.....",
"...........a.####.",
"..........#......#",
"..........#.......",
"..........#.......",
"..........#.......",
".........#........",
".........#........"};


    static const char* const image9_data[] = { 
"17 17 3 1",
". c None",
"# c #000000",
"a c #ff0000",
"........#........",
"........#........",
"........#........",
"........#........",
"........#........",
"......aaaaa......",
".....a..a..a.....",
".....a..a..a.....",
"#####aaaaaaa#####",
".....a..a..a.....",
".....a..a..a.....",
"......aaaaa......",
"........#........",
"........#........",
"........#........",
"........#........",
"........#........"};


    static const char* const image10_data[] = { 
"17 17 3 1",
". c None",
"a c #000000",
"# c #ff0000",
".................",
".................",
".................",
".................",
".................",
"......#####......",
".....#..#..#.....",
".....#..#..#.....",
"aaaaa#######aaaaa",
".....#..#..#.....",
".....#..#..#.....",
"......#####......",
".................",
".................",
".................",
".................",
"................."};


    static const char* const image11_data[] = { 
"17 17 3 1",
". c None",
"# c #000000",
"a c #ff0000",
"........#........",
"........#........",
"........#........",
"........#........",
"........#........",
"......aaaaa......",
".....a..a..a.....",
".....a..a..a.....",
".....aaaaaaa.....",
".....a..a..a.....",
".....a..a..a.....",
"......aaaaa......",
"........#........",
"........#........",
"........#........",
"........#........",
"........#........"};


    static const char* const image12_data[] = { 
"17 17 4 1",
". c None",
"a c #000000",
"# c #ff0000",
"b c #ffffff",
".................",
"..#####..........",
".#..#..#.........",
".#..#..#.........",
".###a###.........",
".#..aa.#.........",
".#..aba#.........",
"..##abba.........",
"....abbba........",
"....abbbba.......",
"....abbbaaa......",
"....ababa........",
"....aaabba.......",
"....a..aba.......",
"........aba......",
"........aba......",
".........a......."};


    static const char* const image13_data[] = { 
"17 17 3 1",
". c None",
"# c #000000",
"a c #ff0000",
"............##...",
"...........#..#..",
".###########..#..",
".###########..#..",
"..##.......#..#..",
"..##........##...",
"..##.............",
".................",
"......aaaaa......",
".....a..a..a.....",
".....a..a..a.....",
".....aaaaaaa.....",
".....a..a..a.....",
".....a..a..a.....",
"......aaaaa......",
".................",
"................."};


    static const char* const image14_data[] = { 
"18 18 3 1",
". c None",
"# c #000000",
"a c #ff0000",
"..................",
"..................",
"..............#...",
"..............#...",
"..............#...",
"..............#...",
".............#....",
".............#....",
".............#....",
".#................",
"..####............",
"......###...a.....",
"...........aaa....",
"............a.....",
"..................",
"..................",
"..................",
".................."};


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
        default: return QPixmap();
    } // switch
    } // icon

};

namespace Ui {
    class QG_CadToolBarSnap: public Ui_QG_CadToolBarSnap {};
} // namespace Ui

QT_END_NAMESPACE

class QG_CadToolBarSnap : public QWidget, public Ui::QG_CadToolBarSnap
{
    Q_OBJECT

public:
    QG_CadToolBarSnap(QWidget* parent = 0, const char* name = 0, Qt::WindowFlags fl = 0);
    ~QG_CadToolBarSnap();

public slots:
    virtual void contextMenuEvent( QContextMenuEvent * e );
    virtual void setCadToolBar( QG_CadToolBar * tb );
    virtual void snapFree();
    virtual void snapGrid();
    virtual void snapEndpoint();
    virtual void snapOnEntity();
    virtual void snapCenter();
    virtual void snapMiddle();
    virtual void snapDist();
    virtual void snapIntersection();
    virtual void snapIntersectionManual();
    virtual void restrictNothing();
    virtual void restrictOrthogonal();
    virtual void restrictHorizontal();
    virtual void restrictVertical();
    virtual void disableSnaps();
    virtual void disableRestrictions();
    virtual void setSnapMode( int sm );
    virtual void setSnapRestriction( int sr );
    virtual void setRelativeZero();
    virtual void lockRelativeZero( bool on );
    virtual void setLockRelativeZero( bool on );
    virtual void back();

protected:
    QG_CadToolBar* cadToolBar;
    QG_ActionHandler* actionHandler;

protected slots:
    virtual void languageChange();

private:
    void init();

};

#endif // QG_CADTOOLBARSNAP_H
