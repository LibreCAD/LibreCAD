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
#ifndef QG_DLGSPLINE_H
#define QG_DLGSPLINE_H

#include <qvariant.h>

class RS_Spline;

#include <Qt3Support/Q3ButtonGroup>
#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QCheckBox>
#include <QtGui/QComboBox>
#include <QtGui/QDialog>
#include <QtGui/QGridLayout>
#include <QtGui/QHBoxLayout>
#include <QtGui/QHeaderView>
#include <QtGui/QLabel>
#include <QtGui/QPushButton>
#include <QtGui/QSpacerItem>
#include <QtGui/QVBoxLayout>
#include "qg_layerbox.h"
#include "qg_widgetpen.h"
#include "rs_pen.h"

QT_BEGIN_NAMESPACE

class Ui_QG_DlgSpline
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
    QSpacerItem *spacer58;
    QSpacerItem *spacer61;
    QLabel *lDegree;
    QCheckBox *cbClosed;
    QComboBox *cbDegree;
    QHBoxLayout *hboxLayout2;
    QSpacerItem *spacer;
    QPushButton *bOk;
    QPushButton *bCancel;

    void setupUi(QDialog *QG_DlgSpline)
    {
        if (QG_DlgSpline->objectName().isEmpty())
            QG_DlgSpline->setObjectName(QString::fromUtf8("QG_DlgSpline"));
        QG_DlgSpline->resize(406, 234);
        QSizePolicy sizePolicy(static_cast<QSizePolicy::Policy>(1), static_cast<QSizePolicy::Policy>(1));
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(QG_DlgSpline->sizePolicy().hasHeightForWidth());
        QG_DlgSpline->setSizePolicy(sizePolicy);
        QG_DlgSpline->setMinimumSize(QSize(300, 190));
        vboxLayout = new QVBoxLayout(QG_DlgSpline);
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
        lLayer = new QLabel(QG_DlgSpline);
        lLayer->setObjectName(QString::fromUtf8("lLayer"));
        QSizePolicy sizePolicy1(static_cast<QSizePolicy::Policy>(5), static_cast<QSizePolicy::Policy>(5));
        sizePolicy1.setHorizontalStretch(0);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(lLayer->sizePolicy().hasHeightForWidth());
        lLayer->setSizePolicy(sizePolicy1);
        lLayer->setWordWrap(false);

        hboxLayout1->addWidget(lLayer);

        cbLayer = new QG_LayerBox(QG_DlgSpline);
        cbLayer->setObjectName(QString::fromUtf8("cbLayer"));

        hboxLayout1->addWidget(cbLayer);


        vboxLayout1->addLayout(hboxLayout1);

        wPen = new QG_WidgetPen(QG_DlgSpline);
        wPen->setObjectName(QString::fromUtf8("wPen"));
        QSizePolicy sizePolicy2(static_cast<QSizePolicy::Policy>(3), static_cast<QSizePolicy::Policy>(3));
        sizePolicy2.setHorizontalStretch(0);
        sizePolicy2.setVerticalStretch(0);
        sizePolicy2.setHeightForWidth(wPen->sizePolicy().hasHeightForWidth());
        wPen->setSizePolicy(sizePolicy2);

        vboxLayout1->addWidget(wPen);


        hboxLayout->addLayout(vboxLayout1);

        buttonGroup8 = new Q3ButtonGroup(QG_DlgSpline);
        buttonGroup8->setObjectName(QString::fromUtf8("buttonGroup8"));
        QSizePolicy sizePolicy3(static_cast<QSizePolicy::Policy>(5), static_cast<QSizePolicy::Policy>(5));
        sizePolicy3.setHorizontalStretch(1);
        sizePolicy3.setVerticalStretch(0);
        sizePolicy3.setHeightForWidth(buttonGroup8->sizePolicy().hasHeightForWidth());
        buttonGroup8->setSizePolicy(sizePolicy3);
        buttonGroup8->setColumnLayout(0, Qt::Vertical);
        buttonGroup8->layout()->setSpacing(6);
        buttonGroup8->layout()->setContentsMargins(11, 11, 11, 11);
        gridLayout = new QGridLayout();
        QBoxLayout *boxlayout = qobject_cast<QBoxLayout *>(buttonGroup8->layout());
        if (boxlayout)
            boxlayout->addLayout(gridLayout);
        gridLayout->setAlignment(Qt::AlignTop);
        gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
        spacer58 = new QSpacerItem(20, 20, QSizePolicy::Minimum, QSizePolicy::Expanding);

        gridLayout->addItem(spacer58, 2, 0, 1, 1);

        spacer61 = new QSpacerItem(20, 20, QSizePolicy::Minimum, QSizePolicy::Expanding);

        gridLayout->addItem(spacer61, 2, 1, 1, 1);

        lDegree = new QLabel(buttonGroup8);
        lDegree->setObjectName(QString::fromUtf8("lDegree"));
        lDegree->setWordWrap(false);

        gridLayout->addWidget(lDegree, 0, 0, 1, 1);

        cbClosed = new QCheckBox(buttonGroup8);
        cbClosed->setObjectName(QString::fromUtf8("cbClosed"));

        gridLayout->addWidget(cbClosed, 1, 0, 1, 2);

        cbDegree = new QComboBox(buttonGroup8);
        cbDegree->setObjectName(QString::fromUtf8("cbDegree"));
        cbDegree->setMinimumSize(QSize(64, 0));

        gridLayout->addWidget(cbDegree, 0, 1, 1, 1);


        hboxLayout->addWidget(buttonGroup8);


        vboxLayout->addLayout(hboxLayout);

        hboxLayout2 = new QHBoxLayout();
        hboxLayout2->setSpacing(6);
        hboxLayout2->setObjectName(QString::fromUtf8("hboxLayout2"));
        spacer = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        hboxLayout2->addItem(spacer);

        bOk = new QPushButton(QG_DlgSpline);
        bOk->setObjectName(QString::fromUtf8("bOk"));
        bOk->setDefault(true);

        hboxLayout2->addWidget(bOk);

        bCancel = new QPushButton(QG_DlgSpline);
        bCancel->setObjectName(QString::fromUtf8("bCancel"));

        hboxLayout2->addWidget(bCancel);


        vboxLayout->addLayout(hboxLayout2);

        QWidget::setTabOrder(bOk, bCancel);

        retranslateUi(QG_DlgSpline);
        QObject::connect(bOk, SIGNAL(clicked()), QG_DlgSpline, SLOT(accept()));
        QObject::connect(bCancel, SIGNAL(clicked()), QG_DlgSpline, SLOT(reject()));

        QMetaObject::connectSlotsByName(QG_DlgSpline);
    } // setupUi

    void retranslateUi(QDialog *QG_DlgSpline)
    {
        QG_DlgSpline->setWindowTitle(QApplication::translate("QG_DlgSpline", "Spline", 0, QApplication::UnicodeUTF8));
        lLayer->setText(QApplication::translate("QG_DlgSpline", "Layer:", 0, QApplication::UnicodeUTF8));
        buttonGroup8->setTitle(QApplication::translate("QG_DlgSpline", "Geometry", 0, QApplication::UnicodeUTF8));
        lDegree->setText(QApplication::translate("QG_DlgSpline", "Degree:", 0, QApplication::UnicodeUTF8));
        cbClosed->setText(QApplication::translate("QG_DlgSpline", "Closed", 0, QApplication::UnicodeUTF8));
        cbDegree->clear();
        cbDegree->insertItems(0, QStringList()
         << QApplication::translate("QG_DlgSpline", "1", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("QG_DlgSpline", "2", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("QG_DlgSpline", "3", 0, QApplication::UnicodeUTF8)
        );
        bOk->setText(QApplication::translate("QG_DlgSpline", "&OK", 0, QApplication::UnicodeUTF8));
        bOk->setShortcut(QApplication::translate("QG_DlgSpline", "Alt+O", 0, QApplication::UnicodeUTF8));
        bCancel->setText(QApplication::translate("QG_DlgSpline", "Cancel", 0, QApplication::UnicodeUTF8));
        bCancel->setShortcut(QApplication::translate("QG_DlgSpline", "Esc", 0, QApplication::UnicodeUTF8));
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
"22 22 142 2",
".b c None",
".# c None",
"Qt c None",
".a c #000000",
".R c #000008",
".j c #000400",
"#b c #000408",
"#2 c #080408",
"#S c #080808",
".Y c #080c10",
"#q c #101010",
"#T c #101408",
"#w c #101410",
"al c #101800",
"#m c #101810",
"#7 c #101c08",
"#I c #101c10",
"#J c #181810",
".1 c #181c10",
"#K c #182010",
"#6 c #182410",
"#P c #202410",
".S c #202810",
"#f c #292c10",
"af c #293000",
"ak c #293008",
"#v c #293010",
"#s c #293018",
".Z c #293418",
".Q c #313010",
".0 c #313818",
"ag c #314008",
"#C c #393818",
".i c #393839",
"#M c #393c18",
".y c #394008",
".7 c #414c18",
".L c #4a5908",
".2 c #4a5918",
"ah c #525d08",
"#z c #525d18",
".5 c #526108",
"aj c #526110",
"#j c #526118",
"#F c #526120",
"#o c #526508",
"#Y c #526520",
"#1 c #5a5d18",
".h c #5a5d5a",
".V c #5a6508",
"ai c #5a6908",
"#A c #5a6910",
".X c #5a6918",
"#a c #5a6920",
"#0 c #5a6d08",
"#O c #5a6d10",
"#u c #626d08",
"aa c #626d20",
"#U c #627108",
"#G c #627110",
"#l c #627120",
".F c #627518",
".G c #627520",
".g c #6a6d6a",
"#4 c #6a7510",
"#c c #6a7520",
"#B c #6a7920",
".P c #6a7d20",
".6 c #737173",
".E c #738920",
"#N c #738929",
".c c #7b797b",
"#x c #7b8920",
"ab c #7b8931",
"#p c #7b8d20",
".f c #837983",
".M c #837d83",
"#R c #838d20",
".8 c #8b9d20",
".O c #8ba110",
".B c #8ba118",
".A c #8ba120",
"#X c #8ba131",
".D c #8ba518",
"#L c #8ba531",
".e c #949194",
".N c #94a520",
"#D c #94a529",
".x c #94a539",
"ae c #94ae18",
".J c #94b218",
"ac c #94b229",
".C c #9cae10",
".H c #9cae18",
"#r c #9cae20",
"#8 c #9cae29",
"ad c #9cae31",
"#g c #9cb220",
"#3 c #9cb229",
".z c #9cb241",
".I c #9cb618",
"## c #9cb629",
"#i c #9cba10",
"#. c #9cba18",
".T c #9cba20",
".K c #a4b618",
".9 c #a4b620",
"#n c #a4b629",
"a# c #a4b631",
"#h c #a4ba10",
".U c #a4ba18",
".3 c #a4ba20",
"#9 c #a4be18",
"#Q c #a4be31",
".d c #aca1ac",
".k c #aca5ac",
".4 c #acbe18",
"#k c #acbe29",
"a. c #acc220",
"#t c #acc229",
"#d c #acc629",
"#V c #acca20",
"#y c #acce20",
".p c #b4be62",
".W c #b4c262",
".o c #b4c26a",
".n c #b4c273",
".r c #b4c662",
".v c #b4ca62",
"#e c #bdc662",
".q c #bdc66a",
".m c #bdc694",
"#5 c #bdca5a",
".w c #bdca62",
".s c #bdca6a",
"#W c #bdda18",
"#E c #bdda20",
".l c #c5bea4",
".t c #c5ca62",
"#H c #c5ce62",
".u c #c5ce6a",
"#Z c #c5e220",
"Qt.#Qt.a.a.a.a.a.a.a.a.a.a.a.a.a.a.a.a.a.a.#",
".bQt.c.d.e.f.g.g.g.g.g.g.g.g.g.g.h.i.j.a.a.a",
"Qt.a.k.l.m.n.o.p.q.r.s.t.u.r.s.v.w.x.y.a.a.a",
".b.a.e.m.z.A.B.C.D.E.F.G.E.H.I.I.J.K.L.a.a.a",
"Qt.a.M.n.N.O.H.P.Q.R.a.a.R.S.P.T.U.I.V.a.a.a",
".b.a.g.W.O.H.X.R.R.Y.Z.0.1.R.R.2.3.4.5.a.a.a",
"Qt.a.6.p.C.P.R.a.7.8.9#.###a#b.a#c#d.V.a.a.a",
".b.a.g#e.D#f.a.7#g#h#i.N#j#k#l.R#m#n#o.a.a.a",
"Qt.a.6.r#p.R#q.8#h#i#r#s.R.7#t#f.a.E#u.a.a.a",
".b.a.g.w.F.a#s.9#i#r#v.R#w#x#y#z.a.X#A.a.a.a",
"Qt.a.6.t#B.a#C#..N#s.R.a.1#D#E#F.a#a#G.a.a.a",
".b.a.g#H.E.R#I###F.R#q#J.a#K#L#M.a#N#O.a.a.a",
"Qt.a.6.r.H#P.R#a#Q.7#R#D#K.a#q#S#T.3#U.a.a.a",
".b.a.g.w.I#c.R#b.G#t#V#W#X#m.a.j#Y#Z#0.a.a.a",
"Qt.a.6.r.I.9#1.a.R#v#1#j#M#2.j.j#P#3#4.a.a.a",
".b.a.h#5.J.U.3#l.1.a.a.a.a#T#Y#6#7#8.5.a.a.a",
"Qt.a.i.x.I.I#9a.a#.Eaa#aab.3#Zacadaeaf.a.a.a",
".b.a.jag.L.5ah.5#oai#A#O#O#O#0#Gajakal.a.a.a",
"Qt.a.a.a.a.a.a.a.a.a.a.a.a.a.a.a.a.a.a.a.a.a",
".b.a.a.a.a.a.a.a.a.a.a.a.a.a.a.a.a.a.a.a.a.a",
"Qt.#.a.a.a.a.a.a.a.a.a.a.a.a.a.a.a.a.a.a.a.a",
".bQt.b.a.a.a.a.a.a.a.a.a.a.a.a.a.a.a.a.a.aQt"};


    switch (id) {
        case image0_ID: return QPixmap((const char**)image0_data);
        case image1_ID: return QPixmap((const char**)image1_data);
        default: return QPixmap();
    } // switch
    } // icon

};

namespace Ui {
    class QG_DlgSpline: public Ui_QG_DlgSpline {};
} // namespace Ui

QT_END_NAMESPACE

class QG_DlgSpline : public QDialog, public Ui::QG_DlgSpline
{
    Q_OBJECT

public:
    QG_DlgSpline(QWidget* parent = 0, const char* name = 0, bool modal = false, Qt::WindowFlags fl = 0);
    ~QG_DlgSpline();

public slots:
    virtual void setSpline( RS_Spline & e );
    virtual void updateSpline();

protected slots:
    virtual void languageChange();

private:
    RS_Spline* spline;

};

#endif // QG_DLGSPLINE_H
