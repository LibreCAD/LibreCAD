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
#ifndef QG_DLGROTATE2_H
#define QG_DLGROTATE2_H

#include <qvariant.h>


#include <Qt3Support/Q3ButtonGroup>
#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QCheckBox>
#include <QtGui/QDialog>
#include <QtGui/QGridLayout>
#include <QtGui/QHBoxLayout>
#include <QtGui/QHeaderView>
#include <QtGui/QLabel>
#include <QtGui/QLineEdit>
#include <QtGui/QPushButton>
#include <QtGui/QRadioButton>
#include <QtGui/QSpacerItem>
#include <QtGui/QVBoxLayout>
#include "rs_modification.h"

QT_BEGIN_NAMESPACE

class Ui_QG_DlgRotate2
{
public:
    QGridLayout *gridLayout;
    QHBoxLayout *hboxLayout;
    QSpacerItem *spacer;
    QPushButton *bOk;
    QPushButton *bCancel;
    Q3ButtonGroup *bgNumber;
    QVBoxLayout *vboxLayout;
    QRadioButton *rbMove;
    QRadioButton *rbCopy;
    QRadioButton *rbMultiCopy;
    QLineEdit *leNumber;
    QSpacerItem *spacer7;
    QVBoxLayout *vboxLayout1;
    QLabel *lHelp;
    QHBoxLayout *hboxLayout1;
    QLabel *lAngle1;
    QSpacerItem *spacer13_2;
    QLineEdit *leAngle1;
    QHBoxLayout *hboxLayout2;
    QLabel *lAngle2;
    QSpacerItem *spacer13;
    QLineEdit *leAngle2;
    QCheckBox *cbCurrentAttributes;
    QCheckBox *cbCurrentLayer;

    void setupUi(QDialog *QG_DlgRotate2)
    {
        if (QG_DlgRotate2->objectName().isEmpty())
            QG_DlgRotate2->setObjectName(QString::fromUtf8("QG_DlgRotate2"));
        QG_DlgRotate2->resize(364, 222);
        QSizePolicy sizePolicy(static_cast<QSizePolicy::Policy>(1), static_cast<QSizePolicy::Policy>(1));
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(QG_DlgRotate2->sizePolicy().hasHeightForWidth());
        QG_DlgRotate2->setSizePolicy(sizePolicy);
        QG_DlgRotate2->setMinimumSize(QSize(300, 190));
        QG_DlgRotate2->setWindowIcon(qt_get_icon(image0_ID));
        gridLayout = new QGridLayout(QG_DlgRotate2);
        gridLayout->setSpacing(6);
        gridLayout->setContentsMargins(11, 11, 11, 11);
        gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
        hboxLayout = new QHBoxLayout();
        hboxLayout->setSpacing(6);
        hboxLayout->setObjectName(QString::fromUtf8("hboxLayout"));
        spacer = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        hboxLayout->addItem(spacer);

        bOk = new QPushButton(QG_DlgRotate2);
        bOk->setObjectName(QString::fromUtf8("bOk"));
        bOk->setDefault(true);

        hboxLayout->addWidget(bOk);

        bCancel = new QPushButton(QG_DlgRotate2);
        bCancel->setObjectName(QString::fromUtf8("bCancel"));

        hboxLayout->addWidget(bCancel);


        gridLayout->addLayout(hboxLayout, 1, 0, 1, 2);

        bgNumber = new Q3ButtonGroup(QG_DlgRotate2);
        bgNumber->setObjectName(QString::fromUtf8("bgNumber"));
        bgNumber->setColumnLayout(0, Qt::Vertical);
        bgNumber->layout()->setSpacing(6);
        bgNumber->layout()->setContentsMargins(11, 11, 11, 11);
        vboxLayout = new QVBoxLayout();
        QBoxLayout *boxlayout = qobject_cast<QBoxLayout *>(bgNumber->layout());
        if (boxlayout)
            boxlayout->addLayout(vboxLayout);
        vboxLayout->setAlignment(Qt::AlignTop);
        vboxLayout->setObjectName(QString::fromUtf8("vboxLayout"));
        rbMove = new QRadioButton(bgNumber);
        rbMove->setObjectName(QString::fromUtf8("rbMove"));
        QSizePolicy sizePolicy1(static_cast<QSizePolicy::Policy>(1), static_cast<QSizePolicy::Policy>(5));
        sizePolicy1.setHorizontalStretch(0);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(rbMove->sizePolicy().hasHeightForWidth());
        rbMove->setSizePolicy(sizePolicy1);
        rbMove->setMinimumSize(QSize(0, 18));

        vboxLayout->addWidget(rbMove);

        rbCopy = new QRadioButton(bgNumber);
        rbCopy->setObjectName(QString::fromUtf8("rbCopy"));
        sizePolicy1.setHeightForWidth(rbCopy->sizePolicy().hasHeightForWidth());
        rbCopy->setSizePolicy(sizePolicy1);
        rbCopy->setMinimumSize(QSize(0, 18));

        vboxLayout->addWidget(rbCopy);

        rbMultiCopy = new QRadioButton(bgNumber);
        rbMultiCopy->setObjectName(QString::fromUtf8("rbMultiCopy"));
        sizePolicy1.setHeightForWidth(rbMultiCopy->sizePolicy().hasHeightForWidth());
        rbMultiCopy->setSizePolicy(sizePolicy1);
        rbMultiCopy->setMinimumSize(QSize(0, 18));

        vboxLayout->addWidget(rbMultiCopy);

        leNumber = new QLineEdit(bgNumber);
        leNumber->setObjectName(QString::fromUtf8("leNumber"));

        vboxLayout->addWidget(leNumber);

        spacer7 = new QSpacerItem(20, 21, QSizePolicy::Minimum, QSizePolicy::Expanding);

        vboxLayout->addItem(spacer7);


        gridLayout->addWidget(bgNumber, 0, 0, 1, 1);

        vboxLayout1 = new QVBoxLayout();
        vboxLayout1->setSpacing(6);
        vboxLayout1->setObjectName(QString::fromUtf8("vboxLayout1"));
        lHelp = new QLabel(QG_DlgRotate2);
        lHelp->setObjectName(QString::fromUtf8("lHelp"));
        lHelp->setPixmap(qt_get_icon(image1_ID));
        lHelp->setAlignment(Qt::AlignCenter);
        lHelp->setWordWrap(false);

        vboxLayout1->addWidget(lHelp);

        hboxLayout1 = new QHBoxLayout();
        hboxLayout1->setSpacing(6);
        hboxLayout1->setObjectName(QString::fromUtf8("hboxLayout1"));
        lAngle1 = new QLabel(QG_DlgRotate2);
        lAngle1->setObjectName(QString::fromUtf8("lAngle1"));
        lAngle1->setWordWrap(false);

        hboxLayout1->addWidget(lAngle1);

        spacer13_2 = new QSpacerItem(41, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        hboxLayout1->addItem(spacer13_2);

        leAngle1 = new QLineEdit(QG_DlgRotate2);
        leAngle1->setObjectName(QString::fromUtf8("leAngle1"));

        hboxLayout1->addWidget(leAngle1);


        vboxLayout1->addLayout(hboxLayout1);

        hboxLayout2 = new QHBoxLayout();
        hboxLayout2->setSpacing(6);
        hboxLayout2->setObjectName(QString::fromUtf8("hboxLayout2"));
        lAngle2 = new QLabel(QG_DlgRotate2);
        lAngle2->setObjectName(QString::fromUtf8("lAngle2"));
        lAngle2->setWordWrap(false);

        hboxLayout2->addWidget(lAngle2);

        spacer13 = new QSpacerItem(41, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        hboxLayout2->addItem(spacer13);

        leAngle2 = new QLineEdit(QG_DlgRotate2);
        leAngle2->setObjectName(QString::fromUtf8("leAngle2"));

        hboxLayout2->addWidget(leAngle2);


        vboxLayout1->addLayout(hboxLayout2);

        cbCurrentAttributes = new QCheckBox(QG_DlgRotate2);
        cbCurrentAttributes->setObjectName(QString::fromUtf8("cbCurrentAttributes"));

        vboxLayout1->addWidget(cbCurrentAttributes);

        cbCurrentLayer = new QCheckBox(QG_DlgRotate2);
        cbCurrentLayer->setObjectName(QString::fromUtf8("cbCurrentLayer"));

        vboxLayout1->addWidget(cbCurrentLayer);


        gridLayout->addLayout(vboxLayout1, 0, 1, 1, 1);

#ifndef QT_NO_SHORTCUT
        lAngle1->setBuddy(leAngle1);
        lAngle2->setBuddy(leAngle2);
#endif // QT_NO_SHORTCUT

        retranslateUi(QG_DlgRotate2);
        QObject::connect(rbMove, SIGNAL(toggled(bool)), leNumber, SLOT(setDisabled(bool)));
        QObject::connect(rbCopy, SIGNAL(toggled(bool)), leNumber, SLOT(setDisabled(bool)));
        QObject::connect(rbMultiCopy, SIGNAL(toggled(bool)), leNumber, SLOT(setEnabled(bool)));
        QObject::connect(bOk, SIGNAL(clicked()), QG_DlgRotate2, SLOT(accept()));
        QObject::connect(bCancel, SIGNAL(clicked()), QG_DlgRotate2, SLOT(reject()));

        QMetaObject::connectSlotsByName(QG_DlgRotate2);
    } // setupUi

    void retranslateUi(QDialog *QG_DlgRotate2)
    {
        QG_DlgRotate2->setWindowTitle(QApplication::translate("QG_DlgRotate2", "Rotate Two Options", 0, QApplication::UnicodeUTF8));
        bOk->setText(QApplication::translate("QG_DlgRotate2", "&OK", 0, QApplication::UnicodeUTF8));
        bCancel->setText(QApplication::translate("QG_DlgRotate2", "Cancel", 0, QApplication::UnicodeUTF8));
        bCancel->setShortcut(QString());
        bgNumber->setTitle(QApplication::translate("QG_DlgRotate2", "Number of copies", 0, QApplication::UnicodeUTF8));
        rbMove->setText(QApplication::translate("QG_DlgRotate2", "&Delete Original", 0, QApplication::UnicodeUTF8));
        rbCopy->setText(QApplication::translate("QG_DlgRotate2", "&Keep Original", 0, QApplication::UnicodeUTF8));
        rbMultiCopy->setText(QApplication::translate("QG_DlgRotate2", "&Multiple Copies", 0, QApplication::UnicodeUTF8));
        lHelp->setText(QString());
        lAngle1->setText(QApplication::translate("QG_DlgRotate2", "Angle (&a):", 0, QApplication::UnicodeUTF8));
        lAngle2->setText(QApplication::translate("QG_DlgRotate2", "Angle (&b):", 0, QApplication::UnicodeUTF8));
        cbCurrentAttributes->setText(QApplication::translate("QG_DlgRotate2", "Use current &attributes", 0, QApplication::UnicodeUTF8));
        cbCurrentLayer->setText(QApplication::translate("QG_DlgRotate2", "Use current &layer", 0, QApplication::UnicodeUTF8));
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


    static const char* const image1_data[] = { 
"88 58 5 1",
". c None",
"a c #000000",
"b c #0000ff",
"c c #838183",
"# c #ff0000",
"........................................................................................",
"........................................................................................",
"........................................................................................",
"..................................#.....................................................",
"........................................................................................",
"..............................aaaaaaaaa......................................b..........",
"..............................a.......a.....................................b...........",
"..............................a.......a....................................b............",
"..............................a.......a...................................b.............",
"..............................a.......a..................................b.b............",
"..............................a.......a.................................b...b...........",
"..............................a...#...a................................b.....b..........",
"..............................a..###..a...............................ba......b.........",
"..............................a...#...a..............................b.a......b.........",
"..............................a.......a.............................b..a.......b........",
".....aaaaaaaaa................a.......a.................aaccaaaaa..b...aaaa....b........",
"..b..a.......a................a.......a.................ac..c...a.b....a...a...b........",
"...b.a.......a................a.......a.................c....c..ab.....a...a....b.......",
".....a.......a................a.......a................ca.....c.a......a...a....b.......",
".....a.......a................aaaaaaaaa...............c.a......ca......a...a..bbbbb.....",
".....a.......a.......................................c..a.....b.c......aaaa....bbb......",
".....a...#...a....................#..................c..a...#b..ac..............b.......",
".....a..###..a....................#...................c.a..###bbabcbbbbbbbbbbbbbbbbbb...",
".....a...#...a....................#....................ca...#...a..c....................",
".....a.......a....................#.....................c.......a..c....................",
".....a.......a....................#.....................ac......a.c.....................",
".....a.......a....................#.....................a.c.....ac......................",
".....a.......a....................#....................ba..c....c.......................",
".....a.......a....................#...................b.a...c..ca.......................",
".....aaaaaaaaa.b..................#..................b..aaaaaccaa.......................",
"................b.................#.................b...................................",
".................b................#................b....................................",
"..................b...............#...............b.....................................",
"...................b..............#..............b......................................",
"....................b.............#.............b.......................................",
".....................b............#............b.bbbb...................................",
"......................b...........#...........b..bbb....................................",
".......................b..........#..........b...bbb....................................",
"........................b.........#.........b....b..b...................................",
".........................b........#........b........b...................................",
"..........................b.......#.......b..........b..................................",
"...........................b......#......b.....aaa...b..........aaaaaaaaa...............",
"............................b.....#.....b.........a..b..........a.......a...............",
".............................b....#....b.......aaaa...b.........a.......a...............",
"..............................b...#...b.......a...a...b.........a.......a...............",
"...............................b..#..b........a...a...b.........a.......a...............",
"................................b.#.b..........aaaa...b.........a.......a...............",
".................................b#b..................b.........a...#...a...............",
"..............................########################b########.a..###..a.####..........",
"..................................#.............................a...#...a...............",
"..................................#.............................a.......a...............",
"..................................#.............................a.......a...............",
"..................................#.............................a.......a...............",
"................................................................a.......a...............",
"................................................................a.......a...............",
"................................................................aaaaaaaaa...............",
"........................................................................................",
"........................................................................................"};


    switch (id) {
        case image0_ID: return QPixmap((const char**)image0_data);
        case image1_ID: return QPixmap((const char**)image1_data);
        default: return QPixmap();
    } // switch
    } // icon

};

namespace Ui {
    class QG_DlgRotate2: public Ui_QG_DlgRotate2 {};
} // namespace Ui

QT_END_NAMESPACE

class QG_DlgRotate2 : public QDialog, public Ui::QG_DlgRotate2
{
    Q_OBJECT

public:
    QG_DlgRotate2(QWidget* parent = 0, const char* name = 0, bool modal = false, Qt::WindowFlags fl = 0);
    ~QG_DlgRotate2();

public slots:
    virtual void setData( RS_Rotate2Data * d );
    virtual void updateData();

protected:
    int newVariable;

protected slots:
    virtual void languageChange();

private:
    QString angle2;
    bool useCurrentAttributes;
    bool useCurrentLayer;
    int numberMode;
    QString copies;
    RS_Rotate2Data* data;
    QString angle1;

    void init();
    void destroy();

};

#endif // QG_DLGROTATE2_H
