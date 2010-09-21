#ifndef QG_CADTOOLBARELLIPSES_H
#define QG_CADTOOLBARELLIPSES_H

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

class Ui_QG_CadToolBarEllipses
{
public:
    QToolButton *bEllipseArcAxes;
    QToolButton *bEllipseAxes;
    QToolButton *bBack;

    void setupUi(QWidget *QG_CadToolBarEllipses)
    {
        if (QG_CadToolBarEllipses->objectName().isEmpty())
            QG_CadToolBarEllipses->setObjectName(QString::fromUtf8("QG_CadToolBarEllipses"));
        QG_CadToolBarEllipses->resize(56, 336);
        QSizePolicy sizePolicy(static_cast<QSizePolicy::Policy>(0), static_cast<QSizePolicy::Policy>(1));
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(QG_CadToolBarEllipses->sizePolicy().hasHeightForWidth());
        QG_CadToolBarEllipses->setSizePolicy(sizePolicy);
        QG_CadToolBarEllipses->setMinimumSize(QSize(56, 336));
        bEllipseArcAxes = new QToolButton(QG_CadToolBarEllipses);
        bEllipseArcAxes->setObjectName(QString::fromUtf8("bEllipseArcAxes"));
        bEllipseArcAxes->setGeometry(QRect(28, 20, 28, 28));
        bEllipseArcAxes->setIcon(qt_get_icon(image0_ID));
        bEllipseAxes = new QToolButton(QG_CadToolBarEllipses);
        bEllipseAxes->setObjectName(QString::fromUtf8("bEllipseAxes"));
        bEllipseAxes->setGeometry(QRect(0, 20, 28, 28));
        bEllipseAxes->setIcon(qt_get_icon(image1_ID));
        bBack = new QToolButton(QG_CadToolBarEllipses);
        bBack->setObjectName(QString::fromUtf8("bBack"));
        bBack->setGeometry(QRect(0, 0, 56, 20));
        bBack->setIcon(qt_get_icon(image2_ID));

        retranslateUi(QG_CadToolBarEllipses);
        QObject::connect(bEllipseAxes, SIGNAL(clicked()), QG_CadToolBarEllipses, SLOT(drawEllipseAxis()));
        QObject::connect(bEllipseArcAxes, SIGNAL(clicked()), QG_CadToolBarEllipses, SLOT(drawEllipseArcAxis()));
        QObject::connect(bBack, SIGNAL(clicked()), QG_CadToolBarEllipses, SLOT(back()));

        QMetaObject::connectSlotsByName(QG_CadToolBarEllipses);
    } // setupUi

    void retranslateUi(QWidget *QG_CadToolBarEllipses)
    {
        QG_CadToolBarEllipses->setWindowTitle(QApplication::translate("QG_CadToolBarEllipses", "Ellipses", 0, QApplication::UnicodeUTF8));
        bEllipseArcAxes->setText(QString());
#ifndef QT_NO_TOOLTIP
        bEllipseArcAxes->setProperty("toolTip", QVariant(QApplication::translate("QG_CadToolBarEllipses", "Ellipse arc with center, two points and angles", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
        bEllipseAxes->setText(QString());
#ifndef QT_NO_TOOLTIP
        bEllipseAxes->setProperty("toolTip", QVariant(QApplication::translate("QG_CadToolBarEllipses", "Ellipse with Center and two points", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
        bBack->setText(QString());
#ifndef QT_NO_TOOLTIP
        bBack->setProperty("toolTip", QVariant(QApplication::translate("QG_CadToolBarEllipses", "Back to main menu", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
    } // retranslateUi


protected:
    enum IconID
    {
        image0_ID,
        image1_ID,
        image2_ID,
        unknown_ID
    };
    static QPixmap qt_get_icon(IconID id)
    {
    static const char* const image0_data[] = { 
"18 18 3 1",
". c None",
"# c #000000",
"a c #ff0000",
".............###aa",
"...........##...aa",
".........##....a.#",
"........#.....a..#",
".......#.....a...#",
".....aa.....a...#.",
".....aa....a....#.",
"....#..a..a....#..",
"...#....aa.....#..",
"..#.....aa....#...",
"..#..........#....",
".#..........#.....",
".#.........#......",
"#.........#.......",
"#.................",
"..................",
"..................",
".................."};


    static const char* const image1_data[] = { 
"18 18 3 1",
". c None",
"# c #000000",
"a c #ff0000",
".............###aa",
"...........##...aa",
".........##....a.#",
"........#.....a..#",
".......#.....a...#",
".....aa.....a...#.",
".....aa....a....#.",
"....#..a..a....#..",
"...#....aa.....#..",
"..#.....aa....#...",
"..#..........#....",
".#..........#.....",
".#.........#......",
"#.........#.......",
"#........#........",
"#......##.........",
".#...##...........",
"..###............."};


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


    switch (id) {
        case image0_ID: return QPixmap((const char**)image0_data);
        case image1_ID: return QPixmap((const char**)image1_data);
        case image2_ID: return QPixmap((const char**)image2_data);
        default: return QPixmap();
    } // switch
    } // icon

};

namespace Ui {
    class QG_CadToolBarEllipses: public Ui_QG_CadToolBarEllipses {};
} // namespace Ui

QT_END_NAMESPACE

class QG_CadToolBarEllipses : public QWidget, public Ui::QG_CadToolBarEllipses
{
    Q_OBJECT

public:
    QG_CadToolBarEllipses(QWidget* parent = 0, const char* name = 0, Qt::WindowFlags fl = 0);
    ~QG_CadToolBarEllipses();

public slots:
    virtual void mousePressEvent( QMouseEvent * e );
    virtual void contextMenuEvent( QContextMenuEvent * e );
    virtual void setCadToolBar( QG_CadToolBar * tb );
    virtual void drawEllipseAxis();
    virtual void drawEllipseArcAxis();
    virtual void back();

protected:
    QG_CadToolBar* cadToolBar;
    QG_ActionHandler* actionHandler;

protected slots:
    virtual void languageChange();

private:
    void init();

};

#endif // QG_CADTOOLBARELLIPSES_H
