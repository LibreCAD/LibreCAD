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
#ifndef QG_DLGROTATE_H
#define QG_DLGROTATE_H

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

class Ui_QG_DlgRotate
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
    QLabel *lAngle;
    QSpacerItem *spacer12;
    QLineEdit *leAngle;
    QCheckBox *cbCurrentAttributes;
    QCheckBox *cbCurrentLayer;

    void setupUi(QDialog *QG_DlgRotate)
    {
        if (QG_DlgRotate->objectName().isEmpty())
            QG_DlgRotate->setObjectName(QString::fromUtf8("QG_DlgRotate"));
        QG_DlgRotate->resize(338, 192);
        QSizePolicy sizePolicy(static_cast<QSizePolicy::Policy>(1), static_cast<QSizePolicy::Policy>(1));
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(QG_DlgRotate->sizePolicy().hasHeightForWidth());
        QG_DlgRotate->setSizePolicy(sizePolicy);
        QG_DlgRotate->setMinimumSize(QSize(300, 190));
        QG_DlgRotate->setWindowIcon(qt_get_icon(image0_ID));
        gridLayout = new QGridLayout(QG_DlgRotate);
        gridLayout->setSpacing(6);
        gridLayout->setContentsMargins(11, 11, 11, 11);
        gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
        hboxLayout = new QHBoxLayout();
        hboxLayout->setSpacing(6);
        hboxLayout->setObjectName(QString::fromUtf8("hboxLayout"));
        spacer = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        hboxLayout->addItem(spacer);

        bOk = new QPushButton(QG_DlgRotate);
        bOk->setObjectName(QString::fromUtf8("bOk"));
        bOk->setDefault(true);

        hboxLayout->addWidget(bOk);

        bCancel = new QPushButton(QG_DlgRotate);
        bCancel->setObjectName(QString::fromUtf8("bCancel"));

        hboxLayout->addWidget(bCancel);


        gridLayout->addLayout(hboxLayout, 1, 0, 1, 2);

        bgNumber = new Q3ButtonGroup(QG_DlgRotate);
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

        spacer7 = new QSpacerItem(20, 146, QSizePolicy::Minimum, QSizePolicy::Expanding);

        vboxLayout->addItem(spacer7);


        gridLayout->addWidget(bgNumber, 0, 0, 1, 1);

        vboxLayout1 = new QVBoxLayout();
        vboxLayout1->setSpacing(6);
        vboxLayout1->setObjectName(QString::fromUtf8("vboxLayout1"));
        lHelp = new QLabel(QG_DlgRotate);
        lHelp->setObjectName(QString::fromUtf8("lHelp"));
        sizePolicy.setHeightForWidth(lHelp->sizePolicy().hasHeightForWidth());
        lHelp->setSizePolicy(sizePolicy);
        lHelp->setMinimumSize(QSize(0, 0));
        lHelp->setPixmap(qt_get_icon(image1_ID));
        lHelp->setAlignment(Qt::AlignCenter);
        lHelp->setWordWrap(false);

        vboxLayout1->addWidget(lHelp);

        hboxLayout1 = new QHBoxLayout();
        hboxLayout1->setSpacing(6);
        hboxLayout1->setObjectName(QString::fromUtf8("hboxLayout1"));
        lAngle = new QLabel(QG_DlgRotate);
        lAngle->setObjectName(QString::fromUtf8("lAngle"));
        lAngle->setWordWrap(false);

        hboxLayout1->addWidget(lAngle);

        spacer12 = new QSpacerItem(31, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        hboxLayout1->addItem(spacer12);

        leAngle = new QLineEdit(QG_DlgRotate);
        leAngle->setObjectName(QString::fromUtf8("leAngle"));
        QSizePolicy sizePolicy2(static_cast<QSizePolicy::Policy>(7), static_cast<QSizePolicy::Policy>(0));
        sizePolicy2.setHorizontalStretch(0);
        sizePolicy2.setVerticalStretch(0);
        sizePolicy2.setHeightForWidth(leAngle->sizePolicy().hasHeightForWidth());
        leAngle->setSizePolicy(sizePolicy2);

        hboxLayout1->addWidget(leAngle);


        vboxLayout1->addLayout(hboxLayout1);

        cbCurrentAttributes = new QCheckBox(QG_DlgRotate);
        cbCurrentAttributes->setObjectName(QString::fromUtf8("cbCurrentAttributes"));

        vboxLayout1->addWidget(cbCurrentAttributes);

        cbCurrentLayer = new QCheckBox(QG_DlgRotate);
        cbCurrentLayer->setObjectName(QString::fromUtf8("cbCurrentLayer"));

        vboxLayout1->addWidget(cbCurrentLayer);


        gridLayout->addLayout(vboxLayout1, 0, 1, 1, 1);

#ifndef QT_NO_SHORTCUT
        lAngle->setBuddy(leAngle);
#endif // QT_NO_SHORTCUT

        retranslateUi(QG_DlgRotate);
        QObject::connect(rbMove, SIGNAL(toggled(bool)), leNumber, SLOT(setDisabled(bool)));
        QObject::connect(rbCopy, SIGNAL(toggled(bool)), leNumber, SLOT(setDisabled(bool)));
        QObject::connect(rbMultiCopy, SIGNAL(toggled(bool)), leNumber, SLOT(setEnabled(bool)));
        QObject::connect(bOk, SIGNAL(clicked()), QG_DlgRotate, SLOT(accept()));
        QObject::connect(bCancel, SIGNAL(clicked()), QG_DlgRotate, SLOT(reject()));

        QMetaObject::connectSlotsByName(QG_DlgRotate);
    } // setupUi

    void retranslateUi(QDialog *QG_DlgRotate)
    {
        QG_DlgRotate->setWindowTitle(QApplication::translate("QG_DlgRotate", "Rotation Options", 0, QApplication::UnicodeUTF8));
        bOk->setText(QApplication::translate("QG_DlgRotate", "&OK", 0, QApplication::UnicodeUTF8));
        bCancel->setText(QApplication::translate("QG_DlgRotate", "&Cancel", 0, QApplication::UnicodeUTF8));
        bCancel->setShortcut(QApplication::translate("QG_DlgRotate", "Esc", 0, QApplication::UnicodeUTF8));
        bgNumber->setTitle(QApplication::translate("QG_DlgRotate", "Number of copies", 0, QApplication::UnicodeUTF8));
        rbMove->setText(QApplication::translate("QG_DlgRotate", "&Delete Original", 0, QApplication::UnicodeUTF8));
        rbCopy->setText(QApplication::translate("QG_DlgRotate", "&Keep Original", 0, QApplication::UnicodeUTF8));
        rbMultiCopy->setText(QApplication::translate("QG_DlgRotate", "&Multiple Copies:", 0, QApplication::UnicodeUTF8));
        lHelp->setText(QString());
        lAngle->setText(QApplication::translate("QG_DlgRotate", "&Angle (a):", 0, QApplication::UnicodeUTF8));
        cbCurrentAttributes->setText(QApplication::translate("QG_DlgRotate", "Use current &attributes", 0, QApplication::UnicodeUTF8));
        cbCurrentLayer->setText(QApplication::translate("QG_DlgRotate", "Use current &layer", 0, QApplication::UnicodeUTF8));
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


    static const char* const image1_data[] = { 
"83 57 4 1",
". c None",
"a c #000000",
"b c #0000ff",
"# c #ff0000",
"...................................................................................",
"...................................................................................",
"....................................#..............................................",
"....................................#..............................................",
"....................................#..............................................",
"....................................#..............................................",
"...................................................................................",
".............................aaaaaaaaaaaaaaa.......................................",
".............................a.............a.......................................",
".............................a.............a.......................................",
".............................a......#......a.......................................",
".............................a.....###.....a.......................................",
".............................a......#......a.......................................",
".............................a.............a.......................................",
".............................a.............a.......................................",
"....b.......aa...............aaaaaaaaaaaaaaa...............aa.......b..............",
".....b.....a..a...........................................a..a.....b...............",
"......b...a....a....................#....................a....a...b................",
".........a......a...................#...................a......a...................",
"........a........a..................#..................a........a..................",
".......a..........a.................#.................a..........a.................",
"......a....#......a.................#.................a......#....a................",
".....a....###....a..................#..................a....###....a...............",
"....a......#....a...................#...................a....#......a..............",
"....a..........a....................#....................a..........a..............",
".....a........a.....................#.....................a........a...............",
"......a......a......................#......................a......a................",
".......a....a...b...................#...................b...a....a.................",
"........a..a.....b..................#..................b.....a..a..................",
".........aa.......b.................#.................b.......aa...................",
"...................b................#................b.............................",
"....................b...............#...............b..............................",
".....................b..............#..............b...............................",
"......................b.............#.............b................................",
".......................b............#............b.bbbb............................",
"........................b...........#...........b..bbb.............................",
".........................b..........#..........b...bbb.............................",
"..........................b.........#.........b....b..b............................",
"...........................b........#........b........b............................",
"............................b.......#.......b..........b...........................",
".............................b......#......b.....aaa...b..........aaaaaaaaa........",
"..............................b.....#.....b.........a..b..........a.......a........",
"...............................b....#....b.......aaaa...b.........a.......a........",
"................................b...#...b.......a...a...b.........a.......a........",
".................................b..#..b........a...a...b.........a.......a........",
"..................................b.#.b..........aaaa...b.........a.......a........",
"...................................b#b..................b.........a...#...a........",
"................................########################b########.a..###..a.####...",
"....................................#.............................a...#...a........",
"....................................#.............................a.......a........",
"....................................#.............................a.......a........",
"....................................#.............................a.......a........",
"..................................................................a.......a........",
"..................................................................a.......a........",
"..................................................................aaaaaaaaa........",
"...................................................................................",
"..................................................................................."};


    switch (id) {
        case image0_ID: return QPixmap((const char**)image0_data);
        case image1_ID: return QPixmap((const char**)image1_data);
        default: return QPixmap();
    } // switch
    } // icon

};

namespace Ui {
    class QG_DlgRotate: public Ui_QG_DlgRotate {};
} // namespace Ui

QT_END_NAMESPACE

class QG_DlgRotate : public QDialog, public Ui::QG_DlgRotate
{
    Q_OBJECT

public:
    QG_DlgRotate(QWidget* parent = 0, const char* name = 0, bool modal = false, Qt::WindowFlags fl = 0);
    ~QG_DlgRotate();

public slots:
    virtual void setData( RS_RotateData * d );
    virtual void updateData();

protected slots:
    virtual void languageChange();

private:
    bool useCurrentAttributes;
    bool useCurrentLayer;
    int numberMode;
    QString copies;
    RS_RotateData* data;
    QString angle;

    void init();
    void destroy();

};

#endif // QG_DLGROTATE_H
