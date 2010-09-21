#ifndef QG_CADTOOLBARMODIFY_H
#define QG_CADTOOLBARMODIFY_H

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

class Ui_QG_CadToolBarModify
{
public:
    QToolButton *bBack;
    QToolButton *bStretch;
    QToolButton *bRound;
    QToolButton *bBevel;
    QToolButton *bTrimAmount;
    QToolButton *bTrim;
    QToolButton *bDelete;
    QToolButton *bAttributes;
    QToolButton *bMirror;
    QToolButton *bScale;
    QToolButton *bRotate;
    QToolButton *bMove;
    QToolButton *bMoveRotate;
    QToolButton *bRotate2;
    QToolButton *bTrim2;
    QToolButton *bCut;
    QToolButton *bExplode;
    QToolButton *bExplodeText;
    QToolButton *bEntity;
    QToolButton *bEntityText;

    void setupUi(QWidget *QG_CadToolBarModify)
    {
        if (QG_CadToolBarModify->objectName().isEmpty())
            QG_CadToolBarModify->setObjectName(QString::fromUtf8("QG_CadToolBarModify"));
        QG_CadToolBarModify->resize(56, 448);
        QSizePolicy sizePolicy(static_cast<QSizePolicy::Policy>(0), static_cast<QSizePolicy::Policy>(1));
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(QG_CadToolBarModify->sizePolicy().hasHeightForWidth());
        QG_CadToolBarModify->setSizePolicy(sizePolicy);
        QG_CadToolBarModify->setMinimumSize(QSize(56, 336));
        bBack = new QToolButton(QG_CadToolBarModify);
        bBack->setObjectName(QString::fromUtf8("bBack"));
        bBack->setGeometry(QRect(0, 0, 56, 20));
        bBack->setIcon(qt_get_icon(image0_ID));
        bStretch = new QToolButton(QG_CadToolBarModify);
        bStretch->setObjectName(QString::fromUtf8("bStretch"));
        bStretch->setGeometry(QRect(28, 206, 28, 28));
        bStretch->setIcon(qt_get_icon(image1_ID));
        bRound = new QToolButton(QG_CadToolBarModify);
        bRound->setObjectName(QString::fromUtf8("bRound"));
        bRound->setGeometry(QRect(28, 172, 28, 28));
        bRound->setIcon(qt_get_icon(image2_ID));
        bBevel = new QToolButton(QG_CadToolBarModify);
        bBevel->setObjectName(QString::fromUtf8("bBevel"));
        bBevel->setGeometry(QRect(0, 172, 28, 28));
        bBevel->setIcon(qt_get_icon(image3_ID));
        bTrimAmount = new QToolButton(QG_CadToolBarModify);
        bTrimAmount->setObjectName(QString::fromUtf8("bTrimAmount"));
        bTrimAmount->setGeometry(QRect(0, 138, 28, 28));
        bTrimAmount->setIcon(qt_get_icon(image4_ID));
        bTrim = new QToolButton(QG_CadToolBarModify);
        bTrim->setObjectName(QString::fromUtf8("bTrim"));
        bTrim->setGeometry(QRect(0, 110, 28, 28));
        bTrim->setIcon(qt_get_icon(image5_ID));
        bDelete = new QToolButton(QG_CadToolBarModify);
        bDelete->setObjectName(QString::fromUtf8("bDelete"));
        bDelete->setGeometry(QRect(0, 268, 28, 28));
        bDelete->setIcon(qt_get_icon(image6_ID));
        bAttributes = new QToolButton(QG_CadToolBarModify);
        bAttributes->setObjectName(QString::fromUtf8("bAttributes"));
        bAttributes->setGeometry(QRect(28, 240, 28, 28));
        bAttributes->setIcon(qt_get_icon(image7_ID));
        bMirror = new QToolButton(QG_CadToolBarModify);
        bMirror->setObjectName(QString::fromUtf8("bMirror"));
        bMirror->setGeometry(QRect(28, 48, 28, 28));
        bMirror->setIcon(qt_get_icon(image8_ID));
        bScale = new QToolButton(QG_CadToolBarModify);
        bScale->setObjectName(QString::fromUtf8("bScale"));
        bScale->setGeometry(QRect(0, 48, 28, 28));
        bScale->setIcon(qt_get_icon(image9_ID));
        bRotate = new QToolButton(QG_CadToolBarModify);
        bRotate->setObjectName(QString::fromUtf8("bRotate"));
        bRotate->setGeometry(QRect(28, 20, 28, 28));
        bRotate->setIcon(qt_get_icon(image10_ID));
        bMove = new QToolButton(QG_CadToolBarModify);
        bMove->setObjectName(QString::fromUtf8("bMove"));
        bMove->setGeometry(QRect(0, 20, 28, 28));
        bMove->setIcon(qt_get_icon(image11_ID));
        bMoveRotate = new QToolButton(QG_CadToolBarModify);
        bMoveRotate->setObjectName(QString::fromUtf8("bMoveRotate"));
        bMoveRotate->setGeometry(QRect(0, 76, 28, 28));
        bMoveRotate->setIcon(qt_get_icon(image12_ID));
        bRotate2 = new QToolButton(QG_CadToolBarModify);
        bRotate2->setObjectName(QString::fromUtf8("bRotate2"));
        bRotate2->setGeometry(QRect(28, 76, 28, 28));
        bRotate2->setIcon(qt_get_icon(image13_ID));
        bTrim2 = new QToolButton(QG_CadToolBarModify);
        bTrim2->setObjectName(QString::fromUtf8("bTrim2"));
        bTrim2->setGeometry(QRect(28, 110, 28, 28));
        bTrim2->setIcon(qt_get_icon(image14_ID));
        bCut = new QToolButton(QG_CadToolBarModify);
        bCut->setObjectName(QString::fromUtf8("bCut"));
        bCut->setGeometry(QRect(0, 206, 28, 28));
        bCut->setIcon(qt_get_icon(image15_ID));
        bExplode = new QToolButton(QG_CadToolBarModify);
        bExplode->setObjectName(QString::fromUtf8("bExplode"));
        bExplode->setGeometry(QRect(28, 268, 28, 28));
        bExplode->setIcon(qt_get_icon(image16_ID));
        bExplodeText = new QToolButton(QG_CadToolBarModify);
        bExplodeText->setObjectName(QString::fromUtf8("bExplodeText"));
        bExplodeText->setGeometry(QRect(0, 302, 28, 28));
        bExplodeText->setIcon(qt_get_icon(image17_ID));
        bEntity = new QToolButton(QG_CadToolBarModify);
        bEntity->setObjectName(QString::fromUtf8("bEntity"));
        bEntity->setGeometry(QRect(0, 240, 28, 28));
        bEntity->setIcon(qt_get_icon(image18_ID));
        bEntityText = new QToolButton(QG_CadToolBarModify);
        bEntityText->setObjectName(QString::fromUtf8("bEntityText"));
        bEntityText->setGeometry(QRect(28, 302, 28, 28));
        bEntityText->setIcon(qt_get_icon(image19_ID));

        retranslateUi(QG_CadToolBarModify);
        QObject::connect(bMove, SIGNAL(clicked()), QG_CadToolBarModify, SLOT(modifyMove()));
        QObject::connect(bBack, SIGNAL(clicked()), QG_CadToolBarModify, SLOT(back()));
        QObject::connect(bRotate, SIGNAL(clicked()), QG_CadToolBarModify, SLOT(modifyRotate()));
        QObject::connect(bEntity, SIGNAL(clicked()), QG_CadToolBarModify, SLOT(modifyEntity()));
        QObject::connect(bScale, SIGNAL(clicked()), QG_CadToolBarModify, SLOT(modifyScale()));
        QObject::connect(bDelete, SIGNAL(clicked()), QG_CadToolBarModify, SLOT(modifyDelete()));
        QObject::connect(bTrim, SIGNAL(clicked()), QG_CadToolBarModify, SLOT(modifyTrim()));
        QObject::connect(bMirror, SIGNAL(clicked()), QG_CadToolBarModify, SLOT(modifyMirror()));
        QObject::connect(bTrim2, SIGNAL(clicked()), QG_CadToolBarModify, SLOT(modifyTrim2()));
        QObject::connect(bMoveRotate, SIGNAL(clicked()), QG_CadToolBarModify, SLOT(modifyMoveRotate()));
        QObject::connect(bTrimAmount, SIGNAL(clicked()), QG_CadToolBarModify, SLOT(modifyTrimAmount()));
        QObject::connect(bCut, SIGNAL(clicked()), QG_CadToolBarModify, SLOT(modifyCut()));
        QObject::connect(bRotate2, SIGNAL(clicked()), QG_CadToolBarModify, SLOT(modifyRotate2()));
        QObject::connect(bStretch, SIGNAL(clicked()), QG_CadToolBarModify, SLOT(modifyStretch()));
        QObject::connect(bBevel, SIGNAL(clicked()), QG_CadToolBarModify, SLOT(modifyBevel()));
        QObject::connect(bRound, SIGNAL(clicked()), QG_CadToolBarModify, SLOT(modifyRound()));
        QObject::connect(bExplode, SIGNAL(clicked()), QG_CadToolBarModify, SLOT(modifyExplode()));
        QObject::connect(bAttributes, SIGNAL(clicked()), QG_CadToolBarModify, SLOT(modifyAttributes()));
        QObject::connect(bExplodeText, SIGNAL(clicked()), QG_CadToolBarModify, SLOT(modifyExplodeText()));
        QObject::connect(bEntityText, SIGNAL(clicked()), QG_CadToolBarModify, SLOT(modifyEntity()));

        QMetaObject::connectSlotsByName(QG_CadToolBarModify);
    } // setupUi

    void retranslateUi(QWidget *QG_CadToolBarModify)
    {
        QG_CadToolBarModify->setWindowTitle(QApplication::translate("QG_CadToolBarModify", "Modify", 0, QApplication::UnicodeUTF8));
        bBack->setText(QString());
#ifndef QT_NO_TOOLTIP
        bBack->setProperty("toolTip", QVariant(QApplication::translate("QG_CadToolBarModify", "Back to main menu", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
        bStretch->setText(QString());
#ifndef QT_NO_TOOLTIP
        bStretch->setProperty("toolTip", QVariant(QApplication::translate("QG_CadToolBarModify", "Stretch", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
        bRound->setText(QString());
#ifndef QT_NO_TOOLTIP
        bRound->setProperty("toolTip", QVariant(QApplication::translate("QG_CadToolBarModify", "Round", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
        bBevel->setText(QString());
#ifndef QT_NO_TOOLTIP
        bBevel->setProperty("toolTip", QVariant(QApplication::translate("QG_CadToolBarModify", "Bevel", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
        bTrimAmount->setText(QString());
#ifndef QT_NO_TOOLTIP
        bTrimAmount->setProperty("toolTip", QVariant(QApplication::translate("QG_CadToolBarModify", "Trim by amount", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
        bTrim->setText(QString());
#ifndef QT_NO_TOOLTIP
        bTrim->setProperty("toolTip", QVariant(QApplication::translate("QG_CadToolBarModify", "Trim / Extend", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
        bDelete->setText(QString());
#ifndef QT_NO_TOOLTIP
        bDelete->setProperty("toolTip", QVariant(QApplication::translate("QG_CadToolBarModify", "Delete", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
        bAttributes->setText(QString());
#ifndef QT_NO_TOOLTIP
        bAttributes->setProperty("toolTip", QVariant(QApplication::translate("QG_CadToolBarModify", "Edit Entity Attributes", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
        bMirror->setText(QString());
#ifndef QT_NO_TOOLTIP
        bMirror->setProperty("toolTip", QVariant(QApplication::translate("QG_CadToolBarModify", "Mirror", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
        bScale->setText(QString());
#ifndef QT_NO_TOOLTIP
        bScale->setProperty("toolTip", QVariant(QApplication::translate("QG_CadToolBarModify", "Scale", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
        bRotate->setText(QString());
#ifndef QT_NO_TOOLTIP
        bRotate->setProperty("toolTip", QVariant(QApplication::translate("QG_CadToolBarModify", "Rotate", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
        bMove->setText(QString());
#ifndef QT_NO_TOOLTIP
        bMove->setProperty("toolTip", QVariant(QApplication::translate("QG_CadToolBarModify", "Move", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
        bMoveRotate->setText(QString());
#ifndef QT_NO_TOOLTIP
        bMoveRotate->setProperty("toolTip", QVariant(QApplication::translate("QG_CadToolBarModify", "Move and Rotate", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
        bRotate2->setText(QString());
#ifndef QT_NO_TOOLTIP
        bRotate2->setProperty("toolTip", QVariant(QApplication::translate("QG_CadToolBarModify", "Rotate around two centers", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
        bTrim2->setText(QString());
#ifndef QT_NO_TOOLTIP
        bTrim2->setProperty("toolTip", QVariant(QApplication::translate("QG_CadToolBarModify", "Trim / Extend two", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
        bCut->setText(QString());
#ifndef QT_NO_TOOLTIP
        bCut->setProperty("toolTip", QVariant(QApplication::translate("QG_CadToolBarModify", "Divide", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
        bExplode->setText(QString());
#ifndef QT_NO_TOOLTIP
        bExplode->setProperty("toolTip", QVariant(QApplication::translate("QG_CadToolBarModify", "Explode", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
        bExplodeText->setText(QString());
#ifndef QT_NO_TOOLTIP
        bExplodeText->setProperty("toolTip", QVariant(QApplication::translate("QG_CadToolBarModify", "Explode Text into Letters", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
        bEntity->setText(QString());
#ifndef QT_NO_TOOLTIP
        bEntity->setProperty("toolTip", QVariant(QApplication::translate("QG_CadToolBarModify", "Edit Entity Geometry", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
        bEntityText->setText(QString());
#ifndef QT_NO_TOOLTIP
        bEntityText->setProperty("toolTip", QVariant(QApplication::translate("QG_CadToolBarModify", "Edit Text", 0, QApplication::UnicodeUTF8)));
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
        image16_ID,
        image17_ID,
        image18_ID,
        image19_ID,
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
"18 18 4 1",
"a c None",
". c #000000",
"b c #0000ff",
"# c #ff0000",
"...........#######",
".aaaaaaaaa.aaaaaa#",
".aaaaaaaaa.aaaaaa#",
".aaaaaaaaa.aaaaaa#",
".aaaaaaaaa.aaaaaa#",
".aaaa......#######",
".aaaa.aaaaaaaaaaaa",
".aaaa.aaaaaaaaaaaa",
".aaaa.aaaaaaaaaaaa",
".aaaa......#######",
".aaaaaaaaa.aaaaaa#",
".aaaaaaaaa.aabaaa#",
".aaaaaaaaa.aabbaa#",
"a.aaaabbbbbbbbbba#",
"a.aaaaaaaa.aabbaa#",
"aa.aaaaaaa.aabaaa#",
"aaa..aaaaa.aaaaaa#",
"aaaaa......#######"};


    static const char* const image2_data[] = { 
"18 17 3 1",
"a c None",
". c #000000",
"# c #ff0000",
".........###aaaaaa",
"aaaaaaaaaaaa##aaaa",
"aaaaaaaaaaaaaa##aa",
"aaaaaaaaaaaaaaa#aa",
"aaaaaaaaaaaaaaaa#a",
"aaaaaaaaaaaaaaaa#a",
"aaaaaaaaaaaaaaaaa#",
"aaaaaaaaaaaaaaaaa#",
"aaaaaaaaaaaaaaaaa#",
"aaaaaaaaaaaaaaaaa.",
"aaaaaaaaaaaaaaaaa.",
"aaaaaaaaaaaaaaaaa.",
"aaaaaaaaaaaaaaaaa.",
"aaaaaaaaaaaaaaaaa.",
"aaaaaaaaaaaaaaaaa.",
"aaaaaaaaaaaaaaaaa.",
"aaaaaaaaaaaaaaaaa."};


    static const char* const image3_data[] = { 
"18 17 3 1",
"a c None",
". c #000000",
"# c #ff0000",
"........#aaaaaaaaa",
"aaaaaaaaa#aaaaaaaa",
"aaaaaaaaaa#aaaaaaa",
"aaaaaaaaaaa#aaaaaa",
"aaaaaaaaaaaa#aaaaa",
"aaaaaaaaaaaaa#aaaa",
"aaaaaaaaaaaaaa#aaa",
"aaaaaaaaaaaaaaa#aa",
"aaaaaaaaaaaaaaaa#a",
"aaaaaaaaaaaaaaaaa#",
"aaaaaaaaaaaaaaaaa.",
"aaaaaaaaaaaaaaaaa.",
"aaaaaaaaaaaaaaaaa.",
"aaaaaaaaaaaaaaaaa.",
"aaaaaaaaaaaaaaaaa.",
"aaaaaaaaaaaaaaaaa.",
"aaaaaaaaaaaaaaaaa."};


    static const char* const image4_data[] = { 
"16 16 4 1",
". c None",
"# c #000000",
"b c #0000ff",
"a c #ff0000",
"...........#....",
"..........#.....",
".........#......",
"........#.......",
".......#........",
"......a.........",
".....a..........",
"....a....b......",
"...a....b.......",
"..a....b........",
".a..b.b..#..#...",
"....bb....##....",
"....bbb...##....",
".........#..#...",
"................",
"................"};


    static const char* const image5_data[] = { 
"18 18 4 1",
"# c None",
". c #000000",
"b c #0000ff",
"a c #ff0000",
"...###############",
"###..#############",
"#####...##########",
"########...#######",
"###########...####",
"##########a###..##",
"#########a######..",
"########a#########",
"#######a##########",
"######a###bbbb####",
"#####a#####bbb####",
"####a######bbb####",
"###.######b##b####",
"##.######b########",
"#.######b#########",
".######b##########",
"######b###########",
"#####b############"};


    static const char* const image6_data[] = { 
"18 17 5 1",
". c None",
"c c #000000",
"b c #830000",
"a c #ff0000",
"# c #ffffff",
".............##...",
"............####..",
"...........aaaa##.",
"..........aaaaaa#.",
".........aaaaaaa#.",
"........aaaaaaab..",
".......aaaaaabbb..",
".......aaaaabbb...",
"......##aaabbb....",
"cc....####abb.....",
"..cc.######b......",
"....cc#####.......",
".......##.........",
"........cc........",
"..........cc......",
"............cc....",
"..............cc.."};


    static const char* const image7_data[] = { 
"18 18 5 1",
". c None",
"# c #000000",
"b c #0000ff",
"c c #00ff00",
"a c #ff0000",
"....#..#..aaaaaaaa",
".#..#..#..aaaaaaaa",
".#..#..#..aaaaaaaa",
".#..#..#..bbbbbbbb",
".#..#.....bbbbbbbb",
"....#..#..bbbbbbbb",
"....#.....cccccccc",
".#..#..#..cccccccc",
".#..#..#..cccccccc",
".#..#..#..........",
".#..#..#..########",
"....#.............",
"....#..#..########",
".#..#.....########",
".#..#..#..........",
".#..#..#..########",
".#..#..#..########",
"....#..#..########"};


    static const char* const image8_data[] = { 
"17 18 4 1",
". c None",
"a c #000000",
"# c #0000ff",
"b c #ff0000",
"........#........",
".....a..#..b.....",
".....a..#..b.....",
"....aa.....bb....",
"....aa.....bb....",
"....aa..#..bb....",
"...a.a..#..b.b...",
"...a.a..#..b.b...",
"...a.a.....b.b...",
"..a..a.....b..b..",
"..a..a..#..b..b..",
"..a..a..#..b..b..",
".a...a..#..b...b.",
".a...a.....b...b.",
".a...a.....b...b.",
"a....a..#..b....b",
"aaaaaa..#..bbbbbb",
"........#........"};


    static const char* const image9_data[] = { 
"18 16 4 1",
"# c None",
". c #000000",
"b c #0000ff",
"a c #ff0000",
".#################",
".#################",
"..##########a#####",
"..##########a#####",
".#.#########aa####",
".#.######bb#aa####",
".##.######bba#a###",
".#bbbbbbbbbbb#a###",
".#bbbbbbbbbbb##a##",
".###.#####bba##a##",
".####.###bb#a###a#",
".####.######a###a#",
".#####.#####a####a",
".#####.#####aaaaaa",
".######.##########",
"........##########"};


    static const char* const image10_data[] = { 
"15 18 4 1",
". c None",
"b c #000000",
"a c #0000ff",
"# c #ff0000",
".............##",
"...........##.#",
".........##...#",
".......##.....#",
".....##..aa...#",
"...#####aaaa###",
"b......aaaaaa..",
"b......a.aa.a..",
"bb.......aa....",
"bb......aaa....",
"b.b.....aa.....",
"b.b...aaaa.....",
"b..aaaaaa......",
"b..aaaa........",
"b...b..........",
"b...b..........",
"b....b.........",
"bbbbbb........."};


    static const char* const image11_data[] = { 
"18 16 4 1",
"# c None",
". c #000000",
"b c #0000ff",
"a c #ff0000",
".#########a#######",
".#########a#######",
"..########aa######",
"..########aa######",
".#.#######a#a#####",
".#.#####bba#a#####",
".##.#####bb##a####",
".#bbbbbbbbbb#a####",
".#bbbbbbbbbb##a###",
".###.####bb###a###",
".####.##bba####a##",
".####.####a####a##",
".#####.###a#####a#",
".#####.###a#####a#",
".######.##a######a",
"........##aaaaaaaa"};


    static const char* const image12_data[] = { 
"18 18 4 1",
". c None",
"a c #000000",
"# c #0000ff",
"b c #ff0000",
"..........###.....",
"...##....####.....",
"....##..###...##..",
"#######.##...####.",
"#######.##..######",
"....##..###...##..",
"...##....######...",
"..........####....",
"..................",
"..................",
"a.................",
"a.................",
"aa............bb..",
"aa..........bb.b..",
"a.a.......bb...b..",
"a.a.....bbbbbbbb..",
"a..a..............",
"aaaa.............."};


    static const char* const image13_data[] = { 
"12 18 4 1",
". c None",
"a c #000000",
"b c #0000ff",
"# c #ff0000",
".......#....",
".......#....",
".......##...",
".......##...",
".......#.#..",
".......#.#..",
".......#..#.",
".......#..#.",
"a......#...#",
"a......#####",
"aa......bb..",
"aa......bb..",
"a.a....bbb..",
"a.a...bbb...",
"a..abbbb....",
"a..abbb.....",
"a...a.......",
"aaaaa......."};


    static const char* const image14_data[] = { 
"18 18 4 1",
". c None",
"# c #000000",
"a c #0000ff",
"b c #ff0000",
".##.......a.......",
"...#.......aa.....",
"....##.......a...a",
"......#.......aaaa",
".......bb......aaa",
".........b....aaaa",
"..........bb......",
"............b.....",
".............bb...",
"...............b..",
".............bb...",
"............b.....",
"..........bb......",
".........b....aaaa",
".......bb......aaa",
"......#.......aaaa",
"....##.......a...a",
"...#.......aa....."};


    static const char* const image15_data[] = { 
"16 16 5 1",
". c None",
"# c #000000",
"b c #626562",
"c c #cdcecd",
"a c #ffffff",
"................",
"................",
"..###...........",
".#...#......##..",
".#....#....#a##.",
"..#b.b#...#a#bb.",
"...###...#a#b...",
".....####a#b....",
".....#b.b#b.....",
".....####c#.....",
"...###bbb#c#....",
"..#b.b#..b#c#...",
".#....#...b#c##.",
".#...#b....b##b.",
".b###b......bb..",
"..bbb..........."};


    static const char* const image16_data[] = { 
"18 18 4 1",
". c None",
"# c #000000",
"a c #c50000",
"b c #ffffff",
"..................",
"..........#..#....",
"...........#.#...#",
"........#.......#.",
".........##..##...",
"............#.....",
"........####...#..",
".......#aa##.#..#.",
"......##a#a#.#....",
".....#a#aaa#......",
"....#aba###.......",
"...#abaaa#........",
"..#abaaa#.........",
".#abaaa#..........",
".#baaa#...........",
".#aaa#............",
"..###.............",
".................."};


    static const char* const image17_data[] = { 
"18 18 3 1",
". c None",
"a c #000000",
"# c #0000ff",
"..................",
"..................",
"..................",
"###..###..###..###",
".#....#....#....#.",
"..................",
"...a...aaa...aaa..",
"..aaa..a..a.aa.aa.",
"..a.a..a..a.a.....",
"..a.a..aaa..a.....",
".aaaaa.a..a.a.....",
".a...a.a..a.aa.aa.",
".a...a.aaa...aaa..",
"..................",
".#....#....#....#.",
"###..###..###..###",
"..................",
".................."};


    static const char* const image18_data[] = { 
"18 18 4 1",
". c None",
"b c #313031",
"# c #5a595a",
"a c #ffffff",
".#######..#######.",
".#aaaaaaa.#aaaaaaa",
".#abbaaaa.#abbaaaa",
"..aaaaaaa..aaaaaaa",
"..................",
"..................",
".#######..#######.",
".#aaaaaaa.#aaaaaaa",
".#abbaaaa.#abbaaaa",
"..aaaaaaa..aaaaaaa",
"..................",
"..................",
"..................",
"aaaaaaa...aaaaaaa.",
"a.b.bb.#..a.b.b.b#",
"ab.bb..#..ab.bb.b#",
"a.b.bb.#..ab.b.b.#",
".#######...#######"};


    static const char* const image19_data[] = { 
"18 18 4 1",
". c None",
"# c #000000",
"b c #d5d2d5",
"a c #ffffff",
"..................",
"..................",
"..................",
".#################",
".#aaaaaaaa#aaaaaab",
".#aaaa#aaa#aaaaaab",
".#aaa#a#aa#aaaaaab",
".#aaa#a#aa#aaaaaab",
".#aaa#a#aa#aaaaaab",
".#aa#aaa#a#aaaaaab",
".#aa#####a#aaaaaab",
".#aa#aaa#a#aaaaaab",
".#a#aaaaa##aaaaaab",
".#a#aaaaa##aaaaaab",
".#aaaaaaaa#aaaaaab",
".#bbbbbbbbbbbbbbbb",
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
        case image15_ID: return QPixmap((const char**)image15_data);
        case image16_ID: return QPixmap((const char**)image16_data);
        case image17_ID: return QPixmap((const char**)image17_data);
        case image18_ID: return QPixmap((const char**)image18_data);
        case image19_ID: return QPixmap((const char**)image19_data);
        default: return QPixmap();
    } // switch
    } // icon

};

namespace Ui {
    class QG_CadToolBarModify: public Ui_QG_CadToolBarModify {};
} // namespace Ui

QT_END_NAMESPACE

class QG_CadToolBarModify : public QWidget, public Ui::QG_CadToolBarModify
{
    Q_OBJECT

public:
    QG_CadToolBarModify(QWidget* parent = 0, const char* name = 0, Qt::WindowFlags fl = 0);
    ~QG_CadToolBarModify();

public slots:
    virtual void mousePressEvent( QMouseEvent * e );
    virtual void contextMenuEvent( QContextMenuEvent * e );
    virtual void setCadToolBar( QG_CadToolBar * tb );
    virtual void modifyMove();
    virtual void modifyRotate();
    virtual void modifyScale();
    virtual void modifyMirror();
    virtual void modifyMoveRotate();
    virtual void modifyRotate2();
    virtual void modifyTrim();
    virtual void modifyTrim2();
    virtual void modifyTrimAmount();
    virtual void modifyCut();
    virtual void modifyBevel();
    virtual void modifyRound();
    virtual void modifyEntity();
    virtual void modifyDelete();
    virtual void modifyAttributes();
    virtual void modifyStretch();
    virtual void modifyExplode();
    virtual void modifyExplodeText();
    virtual void back();

protected:
    QG_ActionHandler* actionHandler;
    QG_CadToolBar* cadToolBar;

protected slots:
    virtual void languageChange();

private:
    void init();

};

#endif // QG_CADTOOLBARMODIFY_H
