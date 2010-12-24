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
#ifndef QG_DLGMOVE_H
#define QG_DLGMOVE_H

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

class Ui_QG_DlgMove
{
public:
    QGridLayout *gridLayout;
    QLabel *lHelp;
    Q3ButtonGroup *bgNumber;
    QVBoxLayout *vboxLayout;
    QRadioButton *rbMove;
    QRadioButton *rbCopy;
    QRadioButton *rbMultiCopy;
    QLineEdit *leNumber;
    QSpacerItem *spacer7;
    QCheckBox *cbCurrentAttributes;
    QCheckBox *cbCurrentLayer;
    QHBoxLayout *hboxLayout;
    QSpacerItem *spacer;
    QPushButton *bOk;
    QPushButton *bCancel;

    void setupUi(QDialog *QG_DlgMove)
    {
        if (QG_DlgMove->objectName().isEmpty())
            QG_DlgMove->setObjectName(QString::fromUtf8("QG_DlgMove"));
        QG_DlgMove->resize(326, 192);
        QSizePolicy sizePolicy(static_cast<QSizePolicy::Policy>(1), static_cast<QSizePolicy::Policy>(1));
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(QG_DlgMove->sizePolicy().hasHeightForWidth());
        QG_DlgMove->setSizePolicy(sizePolicy);
        QG_DlgMove->setMinimumSize(QSize(300, 190));
        QG_DlgMove->setWindowIcon(qt_get_icon(image0_ID));
        gridLayout = new QGridLayout(QG_DlgMove);
        gridLayout->setSpacing(6);
        gridLayout->setContentsMargins(11, 11, 11, 11);
        gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
        lHelp = new QLabel(QG_DlgMove);
        lHelp->setObjectName(QString::fromUtf8("lHelp"));
        lHelp->setPixmap(qt_get_icon(image1_ID));
        lHelp->setAlignment(Qt::AlignCenter);
        lHelp->setWordWrap(false);

        gridLayout->addWidget(lHelp, 0, 1, 1, 1);

        bgNumber = new Q3ButtonGroup(QG_DlgMove);
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

        spacer7 = new QSpacerItem(20, 130, QSizePolicy::Minimum, QSizePolicy::Expanding);

        vboxLayout->addItem(spacer7);


        gridLayout->addWidget(bgNumber, 0, 0, 3, 1);

        cbCurrentAttributes = new QCheckBox(QG_DlgMove);
        cbCurrentAttributes->setObjectName(QString::fromUtf8("cbCurrentAttributes"));

        gridLayout->addWidget(cbCurrentAttributes, 1, 1, 1, 1);

        cbCurrentLayer = new QCheckBox(QG_DlgMove);
        cbCurrentLayer->setObjectName(QString::fromUtf8("cbCurrentLayer"));

        gridLayout->addWidget(cbCurrentLayer, 2, 1, 1, 1);

        hboxLayout = new QHBoxLayout();
        hboxLayout->setSpacing(6);
        hboxLayout->setObjectName(QString::fromUtf8("hboxLayout"));
        spacer = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        hboxLayout->addItem(spacer);

        bOk = new QPushButton(QG_DlgMove);
        bOk->setObjectName(QString::fromUtf8("bOk"));
        bOk->setDefault(true);

        hboxLayout->addWidget(bOk);

        bCancel = new QPushButton(QG_DlgMove);
        bCancel->setObjectName(QString::fromUtf8("bCancel"));

        hboxLayout->addWidget(bCancel);


        gridLayout->addLayout(hboxLayout, 3, 0, 1, 2);


        retranslateUi(QG_DlgMove);
        QObject::connect(rbMove, SIGNAL(toggled(bool)), leNumber, SLOT(setDisabled(bool)));
        QObject::connect(rbCopy, SIGNAL(toggled(bool)), leNumber, SLOT(setDisabled(bool)));
        QObject::connect(rbMultiCopy, SIGNAL(toggled(bool)), leNumber, SLOT(setEnabled(bool)));
        QObject::connect(bOk, SIGNAL(clicked()), QG_DlgMove, SLOT(accept()));
        QObject::connect(bCancel, SIGNAL(clicked()), QG_DlgMove, SLOT(reject()));

        QMetaObject::connectSlotsByName(QG_DlgMove);
    } // setupUi

    void retranslateUi(QDialog *QG_DlgMove)
    {
        QG_DlgMove->setWindowTitle(QApplication::translate("QG_DlgMove", "Moving Options", 0, QApplication::UnicodeUTF8));
        lHelp->setText(QString());
        bgNumber->setTitle(QApplication::translate("QG_DlgMove", "Number of copies", 0, QApplication::UnicodeUTF8));
        rbMove->setText(QApplication::translate("QG_DlgMove", "&Delete Original", 0, QApplication::UnicodeUTF8));
        rbCopy->setText(QApplication::translate("QG_DlgMove", "&Keep Original", 0, QApplication::UnicodeUTF8));
        rbMultiCopy->setText(QApplication::translate("QG_DlgMove", "&Multiple Copies", 0, QApplication::UnicodeUTF8));
        cbCurrentAttributes->setText(QApplication::translate("QG_DlgMove", "Use current &attributes", 0, QApplication::UnicodeUTF8));
        cbCurrentLayer->setText(QApplication::translate("QG_DlgMove", "Use current &layer", 0, QApplication::UnicodeUTF8));
        bOk->setText(QApplication::translate("QG_DlgMove", "&OK", 0, QApplication::UnicodeUTF8));
        bOk->setShortcut(QApplication::translate("QG_DlgMove", "Alt+O", 0, QApplication::UnicodeUTF8));
        bCancel->setText(QApplication::translate("QG_DlgMove", "Cancel", 0, QApplication::UnicodeUTF8));
        bCancel->setShortcut(QApplication::translate("QG_DlgMove", "Esc", 0, QApplication::UnicodeUTF8));
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


    static const char* const image1_data[] = { 
"73 25 3 1",
". c None",
"# c #000000",
"a c #0000ff",
".........................................................................",
".........................................................................",
"..#...................#............#............#........................",
"..#...................#............#............#........................",
"..##..................##...........##...........##.......................",
"..##..................##...........##...........##.......................",
"..#.#.................#.#..........#.#..........#.#......................",
"..#.#.................#.#..........#.#..........#.#......................",
"..#..#................#..#.........#..#.........#..#.....................",
"..#..#..........a.....#..#.........#..#.........#..#.....................",
"..#...#.........aa....#...#........#...#........#...#....................",
"..#...#.........aaa...#...#........#...#........#...#....................",
"..#....#...aaaaaaaaa..#....#.......#....#.......#....#...................",
"..#....#...aaaaaaaaa..#....#.......#....#.......#....#...................",
"..#.....#.......aaa...#.....#......#.....#......#.....#..................",
"..#.....#.......aa....#.....#......#.....#......#.....#..................",
"..#......#......a.....#......#.....#......#.....#......#.................",
"..#......#............#......#.....#......#.....#......#.................",
"..#.......#...........#.......#....#.......#....#.......#................",
"..#.......#...........#.......#....#.......#....#.......#................",
"..#........#..........#........#...#........#...#........#...............",
"..#........#..........#........#...#........#...#........#...............",
"..#.........#.........#.........#..#.........#..#.........#..##..##..##..",
"..###########.........###########..###########..###########..##..##..##..",
"........................................................................."};


    switch (id) {
        case image0_ID: return QPixmap((const char**)image0_data);
        case image1_ID: return QPixmap((const char**)image1_data);
        default: return QPixmap();
    } // switch
    } // icon

};

namespace Ui {
    class QG_DlgMove: public Ui_QG_DlgMove {};
} // namespace Ui

QT_END_NAMESPACE

class QG_DlgMove : public QDialog, public Ui::QG_DlgMove
{
    Q_OBJECT

public:
    QG_DlgMove(QWidget* parent = 0, const char* name = 0, bool modal = false, Qt::WindowFlags fl = 0);
    ~QG_DlgMove();

public slots:
    virtual void setData( RS_MoveData * d );
    virtual void updateData();

protected slots:
    virtual void languageChange();

private:
    bool useCurrentAttributes;
    bool useCurrentLayer;
    int numberMode;
    QString copies;
    RS_MoveData* data;

    void init();
    void destroy();

};

#endif // QG_DLGMOVE_H
