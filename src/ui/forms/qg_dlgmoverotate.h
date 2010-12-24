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
#ifndef QG_DLGMOVEROTATE_H
#define QG_DLGMOVEROTATE_H

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

class Ui_QG_DlgMoveRotate
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
    QLabel *lAngle;
    QSpacerItem *spacer13;
    QLineEdit *leAngle;
    QCheckBox *cbCurrentAttributes;
    QCheckBox *cbCurrentLayer;
    Q3ButtonGroup *bgNumber;
    QVBoxLayout *vboxLayout1;
    QRadioButton *rbMove;
    QRadioButton *rbCopy;
    QRadioButton *rbMultiCopy;
    QLineEdit *leNumber;
    QSpacerItem *spacer7;

    void setupUi(QDialog *QG_DlgMoveRotate)
    {
        if (QG_DlgMoveRotate->objectName().isEmpty())
            QG_DlgMoveRotate->setObjectName(QString::fromUtf8("QG_DlgMoveRotate"));
        QG_DlgMoveRotate->resize(338, 219);
        QSizePolicy sizePolicy(static_cast<QSizePolicy::Policy>(1), static_cast<QSizePolicy::Policy>(1));
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(QG_DlgMoveRotate->sizePolicy().hasHeightForWidth());
        QG_DlgMoveRotate->setSizePolicy(sizePolicy);
        QG_DlgMoveRotate->setMinimumSize(QSize(300, 190));
        QG_DlgMoveRotate->setWindowIcon(qt_get_icon(image0_ID));
        gridLayout = new QGridLayout(QG_DlgMoveRotate);
        gridLayout->setSpacing(6);
        gridLayout->setContentsMargins(11, 11, 11, 11);
        gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
        hboxLayout = new QHBoxLayout();
        hboxLayout->setSpacing(6);
        hboxLayout->setObjectName(QString::fromUtf8("hboxLayout"));
        spacer = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        hboxLayout->addItem(spacer);

        bOk = new QPushButton(QG_DlgMoveRotate);
        bOk->setObjectName(QString::fromUtf8("bOk"));
        bOk->setDefault(true);

        hboxLayout->addWidget(bOk);

        bCancel = new QPushButton(QG_DlgMoveRotate);
        bCancel->setObjectName(QString::fromUtf8("bCancel"));

        hboxLayout->addWidget(bCancel);


        gridLayout->addLayout(hboxLayout, 1, 0, 1, 2);

        vboxLayout = new QVBoxLayout();
        vboxLayout->setSpacing(6);
        vboxLayout->setObjectName(QString::fromUtf8("vboxLayout"));
        lHelp = new QLabel(QG_DlgMoveRotate);
        lHelp->setObjectName(QString::fromUtf8("lHelp"));
        lHelp->setPixmap(qt_get_icon(image1_ID));
        lHelp->setAlignment(Qt::AlignCenter);
        lHelp->setWordWrap(false);

        vboxLayout->addWidget(lHelp);

        hboxLayout1 = new QHBoxLayout();
        hboxLayout1->setSpacing(6);
        hboxLayout1->setObjectName(QString::fromUtf8("hboxLayout1"));
        lAngle = new QLabel(QG_DlgMoveRotate);
        lAngle->setObjectName(QString::fromUtf8("lAngle"));
        lAngle->setWordWrap(false);

        hboxLayout1->addWidget(lAngle);

        spacer13 = new QSpacerItem(41, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        hboxLayout1->addItem(spacer13);

        leAngle = new QLineEdit(QG_DlgMoveRotate);
        leAngle->setObjectName(QString::fromUtf8("leAngle"));

        hboxLayout1->addWidget(leAngle);


        vboxLayout->addLayout(hboxLayout1);

        cbCurrentAttributes = new QCheckBox(QG_DlgMoveRotate);
        cbCurrentAttributes->setObjectName(QString::fromUtf8("cbCurrentAttributes"));

        vboxLayout->addWidget(cbCurrentAttributes);

        cbCurrentLayer = new QCheckBox(QG_DlgMoveRotate);
        cbCurrentLayer->setObjectName(QString::fromUtf8("cbCurrentLayer"));

        vboxLayout->addWidget(cbCurrentLayer);


        gridLayout->addLayout(vboxLayout, 0, 1, 1, 1);

        bgNumber = new Q3ButtonGroup(QG_DlgMoveRotate);
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
        QSizePolicy sizePolicy1(static_cast<QSizePolicy::Policy>(1), static_cast<QSizePolicy::Policy>(5));
        sizePolicy1.setHorizontalStretch(0);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(rbMove->sizePolicy().hasHeightForWidth());
        rbMove->setSizePolicy(sizePolicy1);
        rbMove->setMinimumSize(QSize(0, 18));

        vboxLayout1->addWidget(rbMove);

        rbCopy = new QRadioButton(bgNumber);
        rbCopy->setObjectName(QString::fromUtf8("rbCopy"));
        sizePolicy1.setHeightForWidth(rbCopy->sizePolicy().hasHeightForWidth());
        rbCopy->setSizePolicy(sizePolicy1);
        rbCopy->setMinimumSize(QSize(0, 18));

        vboxLayout1->addWidget(rbCopy);

        rbMultiCopy = new QRadioButton(bgNumber);
        rbMultiCopy->setObjectName(QString::fromUtf8("rbMultiCopy"));
        sizePolicy1.setHeightForWidth(rbMultiCopy->sizePolicy().hasHeightForWidth());
        rbMultiCopy->setSizePolicy(sizePolicy1);
        rbMultiCopy->setMinimumSize(QSize(0, 18));

        vboxLayout1->addWidget(rbMultiCopy);

        leNumber = new QLineEdit(bgNumber);
        leNumber->setObjectName(QString::fromUtf8("leNumber"));

        vboxLayout1->addWidget(leNumber);

        spacer7 = new QSpacerItem(20, 21, QSizePolicy::Minimum, QSizePolicy::Expanding);

        vboxLayout1->addItem(spacer7);


        gridLayout->addWidget(bgNumber, 0, 0, 1, 1);

#ifndef QT_NO_SHORTCUT
        lAngle->setBuddy(leAngle);
#endif // QT_NO_SHORTCUT
        QWidget::setTabOrder(rbMove, rbCopy);
        QWidget::setTabOrder(rbCopy, rbMultiCopy);
        QWidget::setTabOrder(rbMultiCopy, leNumber);
        QWidget::setTabOrder(leNumber, leAngle);
        QWidget::setTabOrder(leAngle, cbCurrentAttributes);
        QWidget::setTabOrder(cbCurrentAttributes, cbCurrentLayer);
        QWidget::setTabOrder(cbCurrentLayer, bOk);
        QWidget::setTabOrder(bOk, bCancel);

        retranslateUi(QG_DlgMoveRotate);
        QObject::connect(rbMove, SIGNAL(toggled(bool)), leNumber, SLOT(setDisabled(bool)));
        QObject::connect(rbCopy, SIGNAL(toggled(bool)), leNumber, SLOT(setDisabled(bool)));
        QObject::connect(rbMultiCopy, SIGNAL(toggled(bool)), leNumber, SLOT(setEnabled(bool)));
        QObject::connect(bOk, SIGNAL(clicked()), QG_DlgMoveRotate, SLOT(accept()));
        QObject::connect(bCancel, SIGNAL(clicked()), QG_DlgMoveRotate, SLOT(reject()));

        QMetaObject::connectSlotsByName(QG_DlgMoveRotate);
    } // setupUi

    void retranslateUi(QDialog *QG_DlgMoveRotate)
    {
        QG_DlgMoveRotate->setWindowTitle(QApplication::translate("QG_DlgMoveRotate", "Move/Rotate Options", 0, QApplication::UnicodeUTF8));
        bOk->setText(QApplication::translate("QG_DlgMoveRotate", "&OK", 0, QApplication::UnicodeUTF8));
        bCancel->setText(QApplication::translate("QG_DlgMoveRotate", "Cancel", 0, QApplication::UnicodeUTF8));
        bCancel->setShortcut(QApplication::translate("QG_DlgMoveRotate", "Esc", 0, QApplication::UnicodeUTF8));
        lHelp->setText(QString());
        lAngle->setText(QApplication::translate("QG_DlgMoveRotate", "&Angle (a):", 0, QApplication::UnicodeUTF8));
        cbCurrentAttributes->setText(QApplication::translate("QG_DlgMoveRotate", "Use current &attributes", 0, QApplication::UnicodeUTF8));
        cbCurrentLayer->setText(QApplication::translate("QG_DlgMoveRotate", "Use current &layer", 0, QApplication::UnicodeUTF8));
        bgNumber->setTitle(QApplication::translate("QG_DlgMoveRotate", "Number of copies", 0, QApplication::UnicodeUTF8));
        rbMove->setText(QApplication::translate("QG_DlgMoveRotate", "&Delete Original", 0, QApplication::UnicodeUTF8));
        rbCopy->setText(QApplication::translate("QG_DlgMoveRotate", "&Keep Original", 0, QApplication::UnicodeUTF8));
        rbMultiCopy->setText(QApplication::translate("QG_DlgMoveRotate", "&Multiple Copies", 0, QApplication::UnicodeUTF8));
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


    static const char* const image1_data[] = { 
"115 41 4 1",
". c None",
"# c #000000",
"b c #0000ff",
"a c #ff0000",
"...................................................................................................................",
"...................................................................................................................",
"..............................................#.....................#..............................................",
"............................................##.#...................#.##............................................",
"..........................................##...#...................#...##..........................................",
".........................................#......#.................#......#.........................................",
"..#####################................##.......#.................#.......##................#####################..",
"..#...................#...............#..........#...............#..........#...............#...................#..",
"..#...................#.............##...........#...............#...........##.............#...................#..",
"..#...................#...........##..............#.............#..............##...........#...................#..",
"..#...................#..........#................#.............#................#..........#...................#..",
"..#...................#........##..................#...........#..................##........#...................#..",
"..#...................#......##.....................#.........#.....................##......#...................#..",
"..#...................#.....#.......................#.........#.......................#.....#...................#..",
"..#.........a.........#......#............a..........#.......#..........a............#......#.........a.........#..",
"..#.........a.........#......#............a..........#.......#..........a............#......#.........a.........#..",
"..#.......aaaaa.......#.......#.........aaaaa.........#.....#.........aaaaa.........#.......#.......aaaaa.......#..",
"..#.........a.........#.......#...........a...........#.....#...........a...........#.......#.........a.........#..",
"..#.........a.........#........#..........a............#...#............a..........#........#.........a.........#..",
"..#...................#.........#......................#...#......................#.........#...................#..",
"..#...................#.........#.......................#.#.......................#.........#...................#..",
"..#...................#..........#....................##...##....................#..........#...................#..",
"..#...................#..........#..................##.......##..................#..........#...................#..",
"..#...................#...........#...............##b..........##...............#...........#...................#..",
"..#...................#...........#.............##...b...........##.............#...........#...................#..",
"..#...................#............#...........#.....b.............#...........#............#...................#..",
"..#####################.............#........##......b..............##........#.............#####################..",
"....................................#......##.........b...............##......#....................................",
".....................................#...##...........b.................##...#.....................................",
".....................................#.##.............b...................##.#.....................................",
"......................................bbbbbbbbbbbbbbbbbbbbbb................#......................................",
"...................................................................................................................",
"...................................................................................................................",
".................................................###...............................................................",
"....................................................#..............................................................",
".................................................####..............................................................",
"................................................#...#..............................................................",
"................................................#...#..............................................................",
".................................................####..............................................................",
"...................................................................................................................",
"..................................................................................................................."};


    switch (id) {
        case image0_ID: return QPixmap((const char**)image0_data);
        case image1_ID: return QPixmap((const char**)image1_data);
        default: return QPixmap();
    } // switch
    } // icon

};

namespace Ui {
    class QG_DlgMoveRotate: public Ui_QG_DlgMoveRotate {};
} // namespace Ui

QT_END_NAMESPACE

class QG_DlgMoveRotate : public QDialog, public Ui::QG_DlgMoveRotate
{
    Q_OBJECT

public:
    QG_DlgMoveRotate(QWidget* parent = 0, const char* name = 0, bool modal = false, Qt::WindowFlags fl = 0);
    ~QG_DlgMoveRotate();

public slots:
    virtual void setData( RS_MoveRotateData * d );
    virtual void updateData();

protected slots:
    virtual void languageChange();

private:
    bool useCurrentAttributes;
    bool useCurrentLayer;
    int numberMode;
    QString copies;
    RS_MoveRotateData* data;
    QString angle;

    void init();
    void destroy();

};

#endif // QG_DLGMOVEROTATE_H
