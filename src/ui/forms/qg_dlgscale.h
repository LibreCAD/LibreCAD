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
#ifndef QG_DLGSCALE_H
#define QG_DLGSCALE_H

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

class Ui_QG_DlgScale
{
public:
    QGridLayout *gridLayout;
    QHBoxLayout *hboxLayout;
    QSpacerItem *spacer;
    QPushButton *bOk;
    QPushButton *bCancel;
    QVBoxLayout *vboxLayout;
    QLabel *lHelp;
    QHBoxLayout *hboxLayout1;
    QLabel *lFactor;
    QSpacerItem *spacer12;
    QLineEdit *leFactor;
    QCheckBox *cbCurrentAttributes;
    QCheckBox *cbCurrentLayer;
    Q3ButtonGroup *bgNumber;
    QVBoxLayout *vboxLayout1;
    QRadioButton *rbMove;
    QRadioButton *rbCopy;
    QRadioButton *rbMultiCopy;
    QLineEdit *leNumber;
    QSpacerItem *spacer7;

    void setupUi(QDialog *QG_DlgScale)
    {
        if (QG_DlgScale->objectName().isEmpty())
            QG_DlgScale->setObjectName(QString::fromUtf8("QG_DlgScale"));
        QG_DlgScale->resize(339, 196);
        QSizePolicy sizePolicy(static_cast<QSizePolicy::Policy>(1), static_cast<QSizePolicy::Policy>(1));
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(QG_DlgScale->sizePolicy().hasHeightForWidth());
        QG_DlgScale->setSizePolicy(sizePolicy);
        QG_DlgScale->setMinimumSize(QSize(300, 190));
        QG_DlgScale->setWindowIcon(qt_get_icon(image0_ID));
        gridLayout = new QGridLayout(QG_DlgScale);
        gridLayout->setSpacing(6);
        gridLayout->setContentsMargins(11, 11, 11, 11);
        gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
        hboxLayout = new QHBoxLayout();
        hboxLayout->setSpacing(6);
        hboxLayout->setObjectName(QString::fromUtf8("hboxLayout"));
        spacer = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        hboxLayout->addItem(spacer);

        bOk = new QPushButton(QG_DlgScale);
        bOk->setObjectName(QString::fromUtf8("bOk"));
        bOk->setDefault(true);

        hboxLayout->addWidget(bOk);

        bCancel = new QPushButton(QG_DlgScale);
        bCancel->setObjectName(QString::fromUtf8("bCancel"));

        hboxLayout->addWidget(bCancel);


        gridLayout->addLayout(hboxLayout, 1, 0, 1, 2);

        vboxLayout = new QVBoxLayout();
        vboxLayout->setSpacing(6);
        vboxLayout->setObjectName(QString::fromUtf8("vboxLayout"));
        lHelp = new QLabel(QG_DlgScale);
        lHelp->setObjectName(QString::fromUtf8("lHelp"));
        lHelp->setPixmap(qt_get_icon(image1_ID));
        lHelp->setAlignment(Qt::AlignCenter);
        lHelp->setWordWrap(false);

        vboxLayout->addWidget(lHelp);

        hboxLayout1 = new QHBoxLayout();
        hboxLayout1->setSpacing(6);
        hboxLayout1->setObjectName(QString::fromUtf8("hboxLayout1"));
        lFactor = new QLabel(QG_DlgScale);
        lFactor->setObjectName(QString::fromUtf8("lFactor"));
        lFactor->setWordWrap(false);

        hboxLayout1->addWidget(lFactor);

        spacer12 = new QSpacerItem(31, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        hboxLayout1->addItem(spacer12);

        leFactor = new QLineEdit(QG_DlgScale);
        leFactor->setObjectName(QString::fromUtf8("leFactor"));
        QSizePolicy sizePolicy1(static_cast<QSizePolicy::Policy>(7), static_cast<QSizePolicy::Policy>(0));
        sizePolicy1.setHorizontalStretch(0);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(leFactor->sizePolicy().hasHeightForWidth());
        leFactor->setSizePolicy(sizePolicy1);

        hboxLayout1->addWidget(leFactor);


        vboxLayout->addLayout(hboxLayout1);

        cbCurrentAttributes = new QCheckBox(QG_DlgScale);
        cbCurrentAttributes->setObjectName(QString::fromUtf8("cbCurrentAttributes"));

        vboxLayout->addWidget(cbCurrentAttributes);

        cbCurrentLayer = new QCheckBox(QG_DlgScale);
        cbCurrentLayer->setObjectName(QString::fromUtf8("cbCurrentLayer"));

        vboxLayout->addWidget(cbCurrentLayer);


        gridLayout->addLayout(vboxLayout, 0, 1, 1, 1);

        bgNumber = new Q3ButtonGroup(QG_DlgScale);
        bgNumber->setObjectName(QString::fromUtf8("bgNumber"));
        bgNumber->setColumnLayout(0, Qt::Vertical);
        bgNumber->layout()->setSpacing(6);
        bgNumber->layout()->setContentsMargins(11, 11, 11, 11);
        vboxLayout1 = new QVBoxLayout();
        QBoxLayout *boxlayout = qobject_cast<QBoxLayout *>(bgNumber->layout());
        if (boxlayout)
            boxlayout->addLayout(vboxLayout1);
        vboxLayout1->setAlignment(Qt::AlignTop);
        vboxLayout1->setObjectName(QString::fromUtf8("vboxLayout1"));
        rbMove = new QRadioButton(bgNumber);
        rbMove->setObjectName(QString::fromUtf8("rbMove"));
        QSizePolicy sizePolicy2(static_cast<QSizePolicy::Policy>(1), static_cast<QSizePolicy::Policy>(5));
        sizePolicy2.setHorizontalStretch(0);
        sizePolicy2.setVerticalStretch(0);
        sizePolicy2.setHeightForWidth(rbMove->sizePolicy().hasHeightForWidth());
        rbMove->setSizePolicy(sizePolicy2);
        rbMove->setMinimumSize(QSize(0, 18));

        vboxLayout1->addWidget(rbMove);

        rbCopy = new QRadioButton(bgNumber);
        rbCopy->setObjectName(QString::fromUtf8("rbCopy"));
        sizePolicy2.setHeightForWidth(rbCopy->sizePolicy().hasHeightForWidth());
        rbCopy->setSizePolicy(sizePolicy2);
        rbCopy->setMinimumSize(QSize(0, 18));

        vboxLayout1->addWidget(rbCopy);

        rbMultiCopy = new QRadioButton(bgNumber);
        rbMultiCopy->setObjectName(QString::fromUtf8("rbMultiCopy"));
        sizePolicy2.setHeightForWidth(rbMultiCopy->sizePolicy().hasHeightForWidth());
        rbMultiCopy->setSizePolicy(sizePolicy2);
        rbMultiCopy->setMinimumSize(QSize(0, 18));

        vboxLayout1->addWidget(rbMultiCopy);

        leNumber = new QLineEdit(bgNumber);
        leNumber->setObjectName(QString::fromUtf8("leNumber"));

        vboxLayout1->addWidget(leNumber);

        spacer7 = new QSpacerItem(20, 16, QSizePolicy::Minimum, QSizePolicy::Expanding);

        vboxLayout1->addItem(spacer7);


        gridLayout->addWidget(bgNumber, 0, 0, 1, 1);

#ifndef QT_NO_SHORTCUT
        lFactor->setBuddy(leFactor);
#endif // QT_NO_SHORTCUT

        retranslateUi(QG_DlgScale);
        QObject::connect(rbMove, SIGNAL(toggled(bool)), leNumber, SLOT(setDisabled(bool)));
        QObject::connect(rbCopy, SIGNAL(toggled(bool)), leNumber, SLOT(setDisabled(bool)));
        QObject::connect(rbMultiCopy, SIGNAL(toggled(bool)), leNumber, SLOT(setEnabled(bool)));
        QObject::connect(bOk, SIGNAL(clicked()), QG_DlgScale, SLOT(accept()));
        QObject::connect(bCancel, SIGNAL(clicked()), QG_DlgScale, SLOT(reject()));

        QMetaObject::connectSlotsByName(QG_DlgScale);
    } // setupUi

    void retranslateUi(QDialog *QG_DlgScale)
    {
        QG_DlgScale->setWindowTitle(QApplication::translate("QG_DlgScale", "Scaling Options", 0, QApplication::UnicodeUTF8));
        bOk->setText(QApplication::translate("QG_DlgScale", "&OK", 0, QApplication::UnicodeUTF8));
        bCancel->setText(QApplication::translate("QG_DlgScale", "&Cancel", 0, QApplication::UnicodeUTF8));
        bCancel->setShortcut(QApplication::translate("QG_DlgScale", "Esc", 0, QApplication::UnicodeUTF8));
        lHelp->setText(QString());
        lFactor->setText(QApplication::translate("QG_DlgScale", "&Factor (f):", 0, QApplication::UnicodeUTF8));
        cbCurrentAttributes->setText(QApplication::translate("QG_DlgScale", "Use current &attributes", 0, QApplication::UnicodeUTF8));
        cbCurrentLayer->setText(QApplication::translate("QG_DlgScale", "Use current &layer", 0, QApplication::UnicodeUTF8));
        bgNumber->setTitle(QApplication::translate("QG_DlgScale", "Number of copies", 0, QApplication::UnicodeUTF8));
        rbMove->setText(QApplication::translate("QG_DlgScale", "&Delete Original", 0, QApplication::UnicodeUTF8));
        rbCopy->setText(QApplication::translate("QG_DlgScale", "&Keep Original", 0, QApplication::UnicodeUTF8));
        rbMultiCopy->setText(QApplication::translate("QG_DlgScale", "&Multiple Copies", 0, QApplication::UnicodeUTF8));
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


    static const char* const image1_data[] = { 
"80 53 4 1",
". c None",
"# c #000000",
"a c #0000ff",
"b c #ff0000",
"................................................................................",
"................................................................................",
".......#a..........................................#...........##...............",
".......#.a........................................##..........#.................",
".......##...aa...................................#.#....##...###................",
".......##.......a..................................#....##....#.................",
".......#.#.......a.................................#..........#.................",
".......#.#..........aa.............................#..........#.................",
".......#..#.............a..........................#....##....#.................",
".......#..#..............a.........................#....##....#.................",
".......#...#................aa.....................#..........#.................",
".......#...#....................a...............................................",
".......#....#....................a..............................................",
".......#....#.......................aa..........................................",
".......#.....#........................#.a.......................................",
".......#.....#........................#..a......................................",
".......#......#.......................##....aa..................................",
".......#......#.......................##........a...............................",
".......#.......#......................#.#........a..............................",
".......#.......#......................#.#...........aa..........................",
".......#........#.....................#..#..............a.......................",
".......#........#.....................#..#...............a......................",
".......#.........#....................#...#.................aa..................",
".......#.........#....................#...#.....................a........b......",
".......#..........#...................#....#.....................a.......b......",
".......#..........#...................#....#........................aa...b......",
".......#...........#..................#.....#.........................bbbbbbb...",
".......#...........#..................#.....#........................a..ab......",
".......#............#.................#......#....................a...a..b......",
".......#............#.................#......#...................a.......b......",
".......#.............#................#.......#..............aa...aa............",
".......#.............#................#.......#...........a.....................",
".......#..............#...............#........#.........a....aa................",
".......#..............#...............#........#.....aa.........................",
".......#...............#..............#.........#aa.......aa....................",
".......#...............#..............#.......a.#...............................",
".......#................#.............#......a...#....aa........................",
".......#................#.............#..aa......#..............................",
".......#.................#............#...........#a............................",
".......#.................#...........a#############.............................",
".......#..................#......aa............aa...............................",
".......#..................#..aa.................................................",
".......#..................a#...............aa...................................",
".......#.................a.#....................................................",
".......#.............aa.....#..........aa.......................................",
".......#..........a.........#...................................................",
".......#.........a...........#.....aa...........................................",
".......#.....aa..............#..................................................",
".......#.aa...................#aa...............................................",
".......########################.................................................",
"................................................................................",
"................................................................................",
"................................................................................"};


    switch (id) {
        case image0_ID: return QPixmap((const char**)image0_data);
        case image1_ID: return QPixmap((const char**)image1_data);
        default: return QPixmap();
    } // switch
    } // icon

};

namespace Ui {
    class QG_DlgScale: public Ui_QG_DlgScale {};
} // namespace Ui

QT_END_NAMESPACE

class QG_DlgScale : public QDialog, public Ui::QG_DlgScale
{
    Q_OBJECT

public:
    QG_DlgScale(QWidget* parent = 0, const char* name = 0, bool modal = false, Qt::WindowFlags fl = 0);
    ~QG_DlgScale();

public slots:
    virtual void setData( RS_ScaleData * d );
    virtual void updateData();

protected slots:
    virtual void languageChange();

private:
    QString factor;
    RS_ScaleData* data;
    QString copies;
    int numberMode;
    bool useCurrentLayer;
    bool useCurrentAttributes;

    void init();
    void destroy();

};

#endif // QG_DLGSCALE_H
