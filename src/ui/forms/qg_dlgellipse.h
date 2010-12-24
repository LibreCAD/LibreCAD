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
#ifndef QG_DLGELLIPSE_H
#define QG_DLGELLIPSE_H

#include <qvariant.h>

class RS_Ellipse;

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
#include <QtGui/QSpacerItem>
#include <QtGui/QVBoxLayout>
#include "qg_layerbox.h"
#include "qg_widgetpen.h"
#include "rs_pen.h"

QT_BEGIN_NAMESPACE

class Ui_QG_DlgEllipse
{
public:
    QVBoxLayout *vboxLayout;
    QHBoxLayout *hboxLayout;
    QVBoxLayout *vboxLayout1;
    QHBoxLayout *hboxLayout1;
    QLabel *lLayer;
    QG_LayerBox *cbLayer;
    QG_WidgetPen *wPen;
    Q3ButtonGroup *buttonGroup8;
    QGridLayout *gridLayout;
    QLabel *lCenterY;
    QLineEdit *leCenterY;
    QLabel *lCenterX;
    QLineEdit *leCenterX;
    QSpacerItem *spacer58;
    QSpacerItem *spacer61;
    QLabel *lAngle2;
    QLineEdit *leAngle2;
    QLineEdit *leAngle1;
    QLabel *lAngle1;
    QLabel *lRotation;
    QLineEdit *leRotation;
    QLineEdit *leMinor;
    QLabel *lMinor;
    QLabel *lMajor;
    QLineEdit *leMajor;
    QCheckBox *cbReversed;
    QHBoxLayout *hboxLayout2;
    QSpacerItem *spacer;
    QPushButton *bOk;
    QPushButton *bCancel;

    void setupUi(QDialog *QG_DlgEllipse)
    {
        if (QG_DlgEllipse->objectName().isEmpty())
            QG_DlgEllipse->setObjectName(QString::fromUtf8("QG_DlgEllipse"));
        QG_DlgEllipse->resize(394, 307);
        QSizePolicy sizePolicy(static_cast<QSizePolicy::Policy>(1), static_cast<QSizePolicy::Policy>(1));
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(QG_DlgEllipse->sizePolicy().hasHeightForWidth());
        QG_DlgEllipse->setSizePolicy(sizePolicy);
        QG_DlgEllipse->setMinimumSize(QSize(300, 190));
        vboxLayout = new QVBoxLayout(QG_DlgEllipse);
        vboxLayout->setSpacing(6);
        vboxLayout->setContentsMargins(11, 11, 11, 11);
        vboxLayout->setObjectName(QString::fromUtf8("vboxLayout"));
        hboxLayout = new QHBoxLayout();
        hboxLayout->setSpacing(6);
        hboxLayout->setObjectName(QString::fromUtf8("hboxLayout"));
        vboxLayout1 = new QVBoxLayout();
        vboxLayout1->setSpacing(6);
        vboxLayout1->setObjectName(QString::fromUtf8("vboxLayout1"));
        hboxLayout1 = new QHBoxLayout();
        hboxLayout1->setSpacing(6);
        hboxLayout1->setObjectName(QString::fromUtf8("hboxLayout1"));
        lLayer = new QLabel(QG_DlgEllipse);
        lLayer->setObjectName(QString::fromUtf8("lLayer"));
        QSizePolicy sizePolicy1(static_cast<QSizePolicy::Policy>(5), static_cast<QSizePolicy::Policy>(5));
        sizePolicy1.setHorizontalStretch(0);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(lLayer->sizePolicy().hasHeightForWidth());
        lLayer->setSizePolicy(sizePolicy1);
        lLayer->setWordWrap(false);

        hboxLayout1->addWidget(lLayer);

        cbLayer = new QG_LayerBox(QG_DlgEllipse);
        cbLayer->setObjectName(QString::fromUtf8("cbLayer"));

        hboxLayout1->addWidget(cbLayer);


        vboxLayout1->addLayout(hboxLayout1);

        wPen = new QG_WidgetPen(QG_DlgEllipse);
        wPen->setObjectName(QString::fromUtf8("wPen"));
        QSizePolicy sizePolicy2(static_cast<QSizePolicy::Policy>(3), static_cast<QSizePolicy::Policy>(3));
        sizePolicy2.setHorizontalStretch(0);
        sizePolicy2.setVerticalStretch(0);
        sizePolicy2.setHeightForWidth(wPen->sizePolicy().hasHeightForWidth());
        wPen->setSizePolicy(sizePolicy2);

        vboxLayout1->addWidget(wPen);


        hboxLayout->addLayout(vboxLayout1);

        buttonGroup8 = new Q3ButtonGroup(QG_DlgEllipse);
        buttonGroup8->setObjectName(QString::fromUtf8("buttonGroup8"));
        buttonGroup8->setColumnLayout(0, Qt::Vertical);
        buttonGroup8->layout()->setSpacing(6);
        buttonGroup8->layout()->setContentsMargins(11, 11, 11, 11);
        gridLayout = new QGridLayout();
        QBoxLayout *boxlayout = qobject_cast<QBoxLayout *>(buttonGroup8->layout());
        if (boxlayout)
            boxlayout->addLayout(gridLayout);
        gridLayout->setAlignment(Qt::AlignTop);
        gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
        lCenterY = new QLabel(buttonGroup8);
        lCenterY->setObjectName(QString::fromUtf8("lCenterY"));
        lCenterY->setWordWrap(false);

        gridLayout->addWidget(lCenterY, 1, 0, 1, 1);

        leCenterY = new QLineEdit(buttonGroup8);
        leCenterY->setObjectName(QString::fromUtf8("leCenterY"));
        leCenterY->setMinimumSize(QSize(64, 0));

        gridLayout->addWidget(leCenterY, 1, 1, 1, 1);

        lCenterX = new QLabel(buttonGroup8);
        lCenterX->setObjectName(QString::fromUtf8("lCenterX"));
        lCenterX->setWordWrap(false);

        gridLayout->addWidget(lCenterX, 0, 0, 1, 1);

        leCenterX = new QLineEdit(buttonGroup8);
        leCenterX->setObjectName(QString::fromUtf8("leCenterX"));
        leCenterX->setMinimumSize(QSize(64, 0));

        gridLayout->addWidget(leCenterX, 0, 1, 1, 1);

        spacer58 = new QSpacerItem(20, 20, QSizePolicy::Minimum, QSizePolicy::Expanding);

        gridLayout->addItem(spacer58, 8, 0, 1, 1);

        spacer61 = new QSpacerItem(20, 20, QSizePolicy::Minimum, QSizePolicy::Expanding);

        gridLayout->addItem(spacer61, 8, 1, 1, 1);

        lAngle2 = new QLabel(buttonGroup8);
        lAngle2->setObjectName(QString::fromUtf8("lAngle2"));
        lAngle2->setWordWrap(false);

        gridLayout->addWidget(lAngle2, 6, 0, 1, 1);

        leAngle2 = new QLineEdit(buttonGroup8);
        leAngle2->setObjectName(QString::fromUtf8("leAngle2"));
        leAngle2->setMinimumSize(QSize(64, 0));

        gridLayout->addWidget(leAngle2, 6, 1, 1, 1);

        leAngle1 = new QLineEdit(buttonGroup8);
        leAngle1->setObjectName(QString::fromUtf8("leAngle1"));
        leAngle1->setMinimumSize(QSize(64, 0));

        gridLayout->addWidget(leAngle1, 5, 1, 1, 1);

        lAngle1 = new QLabel(buttonGroup8);
        lAngle1->setObjectName(QString::fromUtf8("lAngle1"));
        lAngle1->setWordWrap(false);

        gridLayout->addWidget(lAngle1, 5, 0, 1, 1);

        lRotation = new QLabel(buttonGroup8);
        lRotation->setObjectName(QString::fromUtf8("lRotation"));
        lRotation->setWordWrap(false);

        gridLayout->addWidget(lRotation, 4, 0, 1, 1);

        leRotation = new QLineEdit(buttonGroup8);
        leRotation->setObjectName(QString::fromUtf8("leRotation"));
        leRotation->setMinimumSize(QSize(64, 0));

        gridLayout->addWidget(leRotation, 4, 1, 1, 1);

        leMinor = new QLineEdit(buttonGroup8);
        leMinor->setObjectName(QString::fromUtf8("leMinor"));
        leMinor->setMinimumSize(QSize(64, 0));

        gridLayout->addWidget(leMinor, 3, 1, 1, 1);

        lMinor = new QLabel(buttonGroup8);
        lMinor->setObjectName(QString::fromUtf8("lMinor"));
        lMinor->setWordWrap(false);

        gridLayout->addWidget(lMinor, 3, 0, 1, 1);

        lMajor = new QLabel(buttonGroup8);
        lMajor->setObjectName(QString::fromUtf8("lMajor"));
        lMajor->setWordWrap(false);

        gridLayout->addWidget(lMajor, 2, 0, 1, 1);

        leMajor = new QLineEdit(buttonGroup8);
        leMajor->setObjectName(QString::fromUtf8("leMajor"));
        leMajor->setMinimumSize(QSize(64, 0));

        gridLayout->addWidget(leMajor, 2, 1, 1, 1);

        cbReversed = new QCheckBox(buttonGroup8);
        cbReversed->setObjectName(QString::fromUtf8("cbReversed"));

        gridLayout->addWidget(cbReversed, 7, 0, 1, 2);


        hboxLayout->addWidget(buttonGroup8);


        vboxLayout->addLayout(hboxLayout);

        hboxLayout2 = new QHBoxLayout();
        hboxLayout2->setSpacing(6);
        hboxLayout2->setObjectName(QString::fromUtf8("hboxLayout2"));
        spacer = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        hboxLayout2->addItem(spacer);

        bOk = new QPushButton(QG_DlgEllipse);
        bOk->setObjectName(QString::fromUtf8("bOk"));
        bOk->setDefault(true);

        hboxLayout2->addWidget(bOk);

        bCancel = new QPushButton(QG_DlgEllipse);
        bCancel->setObjectName(QString::fromUtf8("bCancel"));

        hboxLayout2->addWidget(bCancel);


        vboxLayout->addLayout(hboxLayout2);

        QWidget::setTabOrder(leCenterX, leCenterY);
        QWidget::setTabOrder(leCenterY, leMajor);
        QWidget::setTabOrder(leMajor, leMinor);
        QWidget::setTabOrder(leMinor, leRotation);
        QWidget::setTabOrder(leRotation, leAngle1);
        QWidget::setTabOrder(leAngle1, leAngle2);
        QWidget::setTabOrder(leAngle2, cbReversed);
        QWidget::setTabOrder(cbReversed, bOk);
        QWidget::setTabOrder(bOk, bCancel);

        retranslateUi(QG_DlgEllipse);
        QObject::connect(bOk, SIGNAL(clicked()), QG_DlgEllipse, SLOT(accept()));
        QObject::connect(bCancel, SIGNAL(clicked()), QG_DlgEllipse, SLOT(reject()));

        QMetaObject::connectSlotsByName(QG_DlgEllipse);
    } // setupUi

    void retranslateUi(QDialog *QG_DlgEllipse)
    {
        QG_DlgEllipse->setWindowTitle(QApplication::translate("QG_DlgEllipse", "Ellipse", 0, QApplication::UnicodeUTF8));
        lLayer->setText(QApplication::translate("QG_DlgEllipse", "Layer:", 0, QApplication::UnicodeUTF8));
        buttonGroup8->setTitle(QApplication::translate("QG_DlgEllipse", "Geometry", 0, QApplication::UnicodeUTF8));
        lCenterY->setText(QApplication::translate("QG_DlgEllipse", "Center (y):", 0, QApplication::UnicodeUTF8));
        lCenterX->setText(QApplication::translate("QG_DlgEllipse", "Center (x):", 0, QApplication::UnicodeUTF8));
        lAngle2->setText(QApplication::translate("QG_DlgEllipse", "End Angle:", 0, QApplication::UnicodeUTF8));
        lAngle1->setText(QApplication::translate("QG_DlgEllipse", "Start Angle:", 0, QApplication::UnicodeUTF8));
        lRotation->setText(QApplication::translate("QG_DlgEllipse", "Rotation:", 0, QApplication::UnicodeUTF8));
        lMinor->setText(QApplication::translate("QG_DlgEllipse", "Minor:", 0, QApplication::UnicodeUTF8));
        lMajor->setText(QApplication::translate("QG_DlgEllipse", "Major:", 0, QApplication::UnicodeUTF8));
        cbReversed->setText(QApplication::translate("QG_DlgEllipse", "Reversed", 0, QApplication::UnicodeUTF8));
        bOk->setText(QApplication::translate("QG_DlgEllipse", "&OK", 0, QApplication::UnicodeUTF8));
        bOk->setShortcut(QApplication::translate("QG_DlgEllipse", "Alt+O", 0, QApplication::UnicodeUTF8));
        bCancel->setText(QApplication::translate("QG_DlgEllipse", "Cancel", 0, QApplication::UnicodeUTF8));
        bCancel->setShortcut(QApplication::translate("QG_DlgEllipse", "Esc", 0, QApplication::UnicodeUTF8));
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
"0 0 0 1"};


    static const char* const image1_data[] = { 
"22 22 114 2",
"Qt c None",
".# c #000000",
".J c #000008",
".g c #000400",
".2 c #000408",
"#J c #080408",
"#y c #080808",
".P c #080c10",
"#c c #101010",
"#z c #101408",
"#h c #101410",
"#V c #101800",
"## c #101810",
"#M c #101c08",
"#p c #101c10",
"#q c #181810",
".S c #181c10",
"#r c #182010",
"#L c #182410",
"#v c #202410",
".K c #202810",
".5 c #292c10",
"#R c #293000",
"#U c #293008",
".I c #293010",
"#e c #293018",
".Q c #293418",
".R c #313818",
".s c #314008",
".f c #393839",
"#t c #393c18",
".W c #414c18",
".F c #4a5908",
".T c #4a5918",
"#I c #525918",
"#S c #525d08",
"#j c #525d18",
".N c #526108",
"#T c #526110",
".9 c #526118",
"#n c #526120",
"#b c #526508",
"#F c #526520",
".e c #5a5d5a",
"#g c #5a6908",
"#k c #5a6910",
".O c #5a6918",
".1 c #5a6920",
"#A c #5a6d08",
"#o c #5a6d10",
"#K c #627110",
".3 c #627120",
".z c #627518",
".A c #627520",
".L c #627920",
".d c #6a6d6a",
"#B c #6a7520",
".H c #6a7d20",
"#P c #738529",
".y c #738920",
"#u c #738929",
".a c #7b797b",
"#x c #7b8920",
".X c #8b9d20",
".G c #8ba110",
".v c #8ba118",
".u c #8ba120",
"#E c #8ba131",
".x c #8ba518",
".8 c #8ba520",
"#s c #8ba531",
".c c #949194",
"#l c #94a529",
".r c #94a539",
".w c #94aa10",
".B c #94aa18",
"#d c #94aa20",
"#N c #94aa29",
"#Q c #94ae18",
".D c #94b218",
".0 c #94b229",
".E c #9cb218",
".Y c #9cb220",
"#a c #9cb229",
".t c #9cb241",
".6 c #9cb610",
".C c #9cb618",
".7 c #9cba10",
".Z c #9cba18",
".M c #9cba20",
"#w c #9cba29",
".b c #a4a1a4",
"#H c #a4b620",
".V c #a4ba18",
".U c #a4ba20",
"#. c #a4ba29",
"#f c #a4be29",
".k c #acbe62",
"#O c #acc220",
".4 c #acc629",
"#C c #acca20",
"#i c #acce20",
".l c #b4be62",
".m c #b4c262",
".j c #b4c273",
".i c #b4c28b",
".q c #b4c65a",
".n c #b4c662",
".p c #b4ca62",
"#D c #b4d618",
"#m c #b4d620",
".h c #bdbea4",
".o c #bdca62",
"#G c #bdde20",
"QtQtQt.#.#.#.#.#.#.#.#.#.#.#.#.#.#.#.#.#.#Qt",
"QtQt.a.b.c.a.d.d.d.d.d.d.d.d.d.d.e.f.g.#.#.#",
"Qt.#.b.h.i.j.k.l.m.n.n.o.o.n.n.p.q.r.s.#.#.#",
"Qt.#.c.i.t.u.v.w.x.y.z.A.y.B.C.D.D.E.F.#.#.#",
"Qt.#.a.j.u.G.B.H.I.J.#.#.J.K.L.M.C.C.N.#.#.#",
"Qt.#.d.k.G.B.O.J.J.P.Q.R.S.J.J.T.U.V.N.#.#.#",
"Qt.#.d.l.w.H.J.#.W.X.Y.Z.0.1.2.#.3.4.N.#.#.#",
"Qt.#.d.m.x.5.#.W.Y.6.7.8.9#..3.J###a#b.#.#.#",
"Qt.#.d.n.y.J#c.X.6.7#d#e.J.W#f.5.#.y#g.#.#.#",
"Qt.#.d.n.z.##e.Y.7#d.I.J#h.y#i#j.#.O#k.#.#.#",
"Qt.#.d.o.A.#.R.Z.u#e.J.#.S#l#m#n.#.1#o.#.#.#",
"Qt.#.d.o.y.J#p.0#n.J#c#q.##r#s#t.##u#o.#.#.#",
"Qt.#.d.n.B#v.J.1#w.W#x#l#r.##c#y#z.U#A.#.#.#",
"Qt.#.d.n.C#B.J.2.A#f#C#D#E##.#.g#F#G#A.#.#.#",
"Qt.#.d.n.D#H#I.#.J.I#I.9#t#J.g.g#v#a#K.#.#.#",
"Qt.#.e.q.D.C.U.3.S.#.#.#.##z#F#L#M#N.N.#.#.#",
"Qt.#.f.r.D.C.Z#O#a.y.1.1#P.U#G.0#N#Q#R.#.#.#",
"Qt.#.g.s.F.N#S.N#b#g#k#o#o#o#A#K#T#U#V.#.#.#",
"Qt.#.#.#.#.#.#.#.#.#.#.#.#.#.#.#.#.#.#.#.#.#",
"Qt.#.#.#.#.#.#.#.#.#.#.#.#.#.#.#.#.#.#.#.#.#",
"QtQt.#.#.#.#.#.#.#.#.#.#.#.#.#.#.#.#.#.#.#.#",
"QtQtQt.#.#.#.#.#.#.#.#.#.#.#.#.#.#.#.#.#.#Qt"};


    switch (id) {
        case image0_ID: return QPixmap((const char**)image0_data);
        case image1_ID: return QPixmap((const char**)image1_data);
        default: return QPixmap();
    } // switch
    } // icon

};

namespace Ui {
    class QG_DlgEllipse: public Ui_QG_DlgEllipse {};
} // namespace Ui

QT_END_NAMESPACE

class QG_DlgEllipse : public QDialog, public Ui::QG_DlgEllipse
{
    Q_OBJECT

public:
    QG_DlgEllipse(QWidget* parent = 0, const char* name = 0, bool modal = false, Qt::WindowFlags fl = 0);
    ~QG_DlgEllipse();

public slots:
    virtual void setEllipse( RS_Ellipse & e );
    virtual void updateEllipse();

protected slots:
    virtual void languageChange();

private:
    RS_Ellipse* ellipse;

};

#endif // QG_DLGELLIPSE_H
