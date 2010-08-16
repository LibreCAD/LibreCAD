/****************************************************************************
**
** This file is part of the CADuntu project, a 2D CAD program
**
** Copyright (C) 2010 R. van Twisk (caduntu@rvt.dds.nl)
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
#ifndef QG_DLGMIRROR_H
#define QG_DLGMIRROR_H

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
#include <QtGui/QPushButton>
#include <QtGui/QRadioButton>
#include <QtGui/QSpacerItem>
#include <QtGui/QVBoxLayout>
#include "rs_modification.h"

QT_BEGIN_NAMESPACE

class Ui_QG_DlgMirror
{
public:
    QGridLayout *gridLayout;
    QLabel *lHelp;
    Q3ButtonGroup *bgNumber;
    QVBoxLayout *vboxLayout;
    QRadioButton *rbMove;
    QRadioButton *rbCopy;
    QSpacerItem *spacer7;
    QCheckBox *cbCurrentAttributes;
    QCheckBox *cbCurrentLayer;
    QHBoxLayout *hboxLayout;
    QSpacerItem *spacer;
    QPushButton *bOk;
    QPushButton *bCancel;

    void setupUi(QDialog *QG_DlgMirror)
    {
        if (QG_DlgMirror->objectName().isEmpty())
            QG_DlgMirror->setObjectName(QString::fromUtf8("QG_DlgMirror"));
        QG_DlgMirror->resize(342, 192);
        QSizePolicy sizePolicy(static_cast<QSizePolicy::Policy>(1), static_cast<QSizePolicy::Policy>(1));
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(QG_DlgMirror->sizePolicy().hasHeightForWidth());
        QG_DlgMirror->setSizePolicy(sizePolicy);
        QG_DlgMirror->setMinimumSize(QSize(300, 190));
        QG_DlgMirror->setWindowIcon(qt_get_icon(image0_ID));
        gridLayout = new QGridLayout(QG_DlgMirror);
        gridLayout->setSpacing(6);
        gridLayout->setContentsMargins(11, 11, 11, 11);
        gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
        lHelp = new QLabel(QG_DlgMirror);
        lHelp->setObjectName(QString::fromUtf8("lHelp"));
        lHelp->setPixmap(qt_get_icon(image1_ID));
        lHelp->setAlignment(Qt::AlignCenter);
        lHelp->setWordWrap(false);

        gridLayout->addWidget(lHelp, 0, 1, 1, 1);

        bgNumber = new Q3ButtonGroup(QG_DlgMirror);
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

        spacer7 = new QSpacerItem(20, 21, QSizePolicy::Minimum, QSizePolicy::Expanding);

        vboxLayout->addItem(spacer7);


        gridLayout->addWidget(bgNumber, 0, 0, 3, 1);

        cbCurrentAttributes = new QCheckBox(QG_DlgMirror);
        cbCurrentAttributes->setObjectName(QString::fromUtf8("cbCurrentAttributes"));

        gridLayout->addWidget(cbCurrentAttributes, 1, 1, 1, 1);

        cbCurrentLayer = new QCheckBox(QG_DlgMirror);
        cbCurrentLayer->setObjectName(QString::fromUtf8("cbCurrentLayer"));

        gridLayout->addWidget(cbCurrentLayer, 2, 1, 1, 1);

        hboxLayout = new QHBoxLayout();
        hboxLayout->setSpacing(6);
        hboxLayout->setObjectName(QString::fromUtf8("hboxLayout"));
        spacer = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        hboxLayout->addItem(spacer);

        bOk = new QPushButton(QG_DlgMirror);
        bOk->setObjectName(QString::fromUtf8("bOk"));
        bOk->setDefault(true);

        hboxLayout->addWidget(bOk);

        bCancel = new QPushButton(QG_DlgMirror);
        bCancel->setObjectName(QString::fromUtf8("bCancel"));

        hboxLayout->addWidget(bCancel);


        gridLayout->addLayout(hboxLayout, 3, 0, 1, 2);


        retranslateUi(QG_DlgMirror);
        QObject::connect(bOk, SIGNAL(clicked()), QG_DlgMirror, SLOT(accept()));
        QObject::connect(bCancel, SIGNAL(clicked()), QG_DlgMirror, SLOT(reject()));

        QMetaObject::connectSlotsByName(QG_DlgMirror);
    } // setupUi

    void retranslateUi(QDialog *QG_DlgMirror)
    {
        QG_DlgMirror->setWindowTitle(QApplication::translate("QG_DlgMirror", "Mirroring Options", 0, QApplication::UnicodeUTF8));
        lHelp->setText(QString());
        bgNumber->setTitle(QApplication::translate("QG_DlgMirror", "Number of copies", 0, QApplication::UnicodeUTF8));
        rbMove->setText(QApplication::translate("QG_DlgMirror", "&Delete Original", 0, QApplication::UnicodeUTF8));
        rbCopy->setText(QApplication::translate("QG_DlgMirror", "&Keep Original", 0, QApplication::UnicodeUTF8));
        cbCurrentAttributes->setText(QApplication::translate("QG_DlgMirror", "Use current &attributes", 0, QApplication::UnicodeUTF8));
        cbCurrentLayer->setText(QApplication::translate("QG_DlgMirror", "Use current &layer", 0, QApplication::UnicodeUTF8));
        bOk->setText(QApplication::translate("QG_DlgMirror", "&OK", 0, QApplication::UnicodeUTF8));
        bCancel->setText(QApplication::translate("QG_DlgMirror", "Cancel", 0, QApplication::UnicodeUTF8));
        bCancel->setShortcut(QApplication::translate("QG_DlgMirror", "Esc", 0, QApplication::UnicodeUTF8));
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


    static const char* const image1_data[] = { 
"53 60 3 1",
". c None",
"a c #000000",
"# c #ff0000",
"..........................#..........................",
"..........................#..........................",
"..........................#..........................",
"..........................#..........................",
".....................................................",
"..........................#..........................",
".....................................................",
"..........................#..........................",
"..........................#..........................",
"..........................#..........................",
"....................a.....#.....a....................",
"....................a...........a....................",
"...................aa.....#.....aa...................",
"...................aa...........aa...................",
"..................a.a.....#.....a.a..................",
"..................a.a.....#.....a.a..................",
".................a..a.....#.....a..a.................",
".................a..a.....#.....a..a.................",
"................a...a...........a...a................",
"................a...a.....#.....a...a................",
"...............a....a...........a....a...............",
"...............a....a.....#.....a....a...............",
"..............a.....a.....#.....a.....a..............",
"..............a.....a.....#.....a.....a..............",
".............a......a.....#.....a......a.............",
".............a......a...........a......a.............",
"............a.......a.....#.....a.......a............",
"............a.......a...........a.......a............",
"...........a........a.....#.....a........a...........",
"...........a........a.....#.....a........a...........",
"..........a.........a.....#.....a.........a..........",
"..........a.........a.....#.....a.........a..........",
".........a..........a...........a..........a.........",
".........a..........a.....#.....a..........a.........",
"........a...........a...........a...........a........",
"........a...........a.....#.....a...........a........",
".......a............a.....#.....a............a.......",
".......a............a.....#.....a............a.......",
"......a.............a.....#.....a.............a......",
"......a.............a...........a.............a......",
".....a..............a.....#.....a..............a.....",
".....a..............a...........a..............a.....",
"....a...............a.....#.....a...............a....",
"....a...............a.....#.....a...............a....",
"...a................a.....#.....a................a...",
"...a................a.....#.....a................a...",
"..a.................a...........a.................a..",
"..a.................a.....#.....a.................a..",
".a..................a...........a..................a.",
".aaaaaaaaaaaaaaaaaaaa.....#.....aaaaaaaaaaaaaaaaaaaa.",
"..........................#..........................",
"..........................#..........................",
"..........................#..........................",
".....................................................",
"..........................#..........................",
".....................................................",
"..........................#..........................",
"..........................#..........................",
"..........................#..........................",
"..........................#.........................."};


    switch (id) {
        case image0_ID: return QPixmap((const char**)image0_data);
        case image1_ID: return QPixmap((const char**)image1_data);
        default: return QPixmap();
    } // switch
    } // icon

};

namespace Ui {
    class QG_DlgMirror: public Ui_QG_DlgMirror {};
} // namespace Ui

QT_END_NAMESPACE

class QG_DlgMirror : public QDialog, public Ui::QG_DlgMirror
{
    Q_OBJECT

public:
    QG_DlgMirror(QWidget* parent = 0, const char* name = 0, bool modal = false, Qt::WindowFlags fl = 0);
    ~QG_DlgMirror();

public slots:
    virtual void setData( RS_MirrorData * d );
    virtual void updateData();

protected slots:
    virtual void languageChange();

private:
    RS_MirrorData* data;
    QString copies;
    int numberMode;
    bool useCurrentLayer;
    bool useCurrentAttributes;

    void init();
    void destroy();

};

#endif // QG_DLGMIRROR_H
