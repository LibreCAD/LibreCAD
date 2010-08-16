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
#ifndef QG_DLGHATCH_H
#define QG_DLGHATCH_H

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
#include <QtGui/QSpacerItem>
#include <QtGui/QVBoxLayout>
#include "qg_graphicview.h"
#include "qg_patternbox.h"
#include "rs_entitycontainer.h"
#include "rs_graphic.h"
#include "rs_hatch.h"
#include "rs_pattern.h"
#include "rs_patternlist.h"
#include "rs_settings.h"

QT_BEGIN_NAMESPACE

class Ui_QG_DlgHatch
{
public:
    QVBoxLayout *vboxLayout;
    QHBoxLayout *hboxLayout;
    Q3ButtonGroup *bgParameter;
    QGridLayout *gridLayout;
    QLineEdit *leScale;
    QLineEdit *leAngle;
    QLabel *lAngle;
    QLabel *lScale;
    QG_PatternBox *cbPattern;
    QCheckBox *cbSolid;
    Q3ButtonGroup *bgPreview;
    QVBoxLayout *vboxLayout1;
    QCheckBox *cbEnablePreview;
    QG_GraphicView *gvPreview;
    QHBoxLayout *hboxLayout1;
    QSpacerItem *Horizontal_Spacing2;
    QPushButton *buttonOk;
    QPushButton *buttonCancel;

    void setupUi(QDialog *QG_DlgHatch)
    {
        if (QG_DlgHatch->objectName().isEmpty())
            QG_DlgHatch->setObjectName(QString::fromUtf8("QG_DlgHatch"));
        QG_DlgHatch->resize(438, 294);
        vboxLayout = new QVBoxLayout(QG_DlgHatch);
        vboxLayout->setSpacing(6);
        vboxLayout->setContentsMargins(11, 11, 11, 11);
        vboxLayout->setObjectName(QString::fromUtf8("vboxLayout"));
        hboxLayout = new QHBoxLayout();
        hboxLayout->setSpacing(6);
        hboxLayout->setObjectName(QString::fromUtf8("hboxLayout"));
        bgParameter = new Q3ButtonGroup(QG_DlgHatch);
        bgParameter->setObjectName(QString::fromUtf8("bgParameter"));
        bgParameter->setColumnLayout(0, Qt::Vertical);
        bgParameter->layout()->setSpacing(6);
        bgParameter->layout()->setContentsMargins(11, 11, 11, 11);
        gridLayout = new QGridLayout();
        QBoxLayout *boxlayout = qobject_cast<QBoxLayout *>(bgParameter->layout());
        if (boxlayout)
            boxlayout->addLayout(gridLayout);
        gridLayout->setAlignment(Qt::AlignTop);
        gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
        leScale = new QLineEdit(bgParameter);
        leScale->setObjectName(QString::fromUtf8("leScale"));

        gridLayout->addWidget(leScale, 2, 1, 1, 1);

        leAngle = new QLineEdit(bgParameter);
        leAngle->setObjectName(QString::fromUtf8("leAngle"));

        gridLayout->addWidget(leAngle, 3, 1, 1, 1);

        lAngle = new QLabel(bgParameter);
        lAngle->setObjectName(QString::fromUtf8("lAngle"));
        lAngle->setWordWrap(false);

        gridLayout->addWidget(lAngle, 3, 0, 1, 1);

        lScale = new QLabel(bgParameter);
        lScale->setObjectName(QString::fromUtf8("lScale"));
        lScale->setWordWrap(false);

        gridLayout->addWidget(lScale, 2, 0, 1, 1);

        cbPattern = new QG_PatternBox(bgParameter);
        cbPattern->setObjectName(QString::fromUtf8("cbPattern"));

        gridLayout->addWidget(cbPattern, 1, 0, 1, 2);

        cbSolid = new QCheckBox(bgParameter);
        cbSolid->setObjectName(QString::fromUtf8("cbSolid"));

        gridLayout->addWidget(cbSolid, 0, 0, 1, 2);


        hboxLayout->addWidget(bgParameter);

        bgPreview = new Q3ButtonGroup(QG_DlgHatch);
        bgPreview->setObjectName(QString::fromUtf8("bgPreview"));
        bgPreview->setColumnLayout(0, Qt::Vertical);
        bgPreview->layout()->setSpacing(6);
        bgPreview->layout()->setContentsMargins(11, 11, 11, 11);
        vboxLayout1 = new QVBoxLayout();
        QBoxLayout *boxlayout1 = qobject_cast<QBoxLayout *>(bgPreview->layout());
        if (boxlayout1)
            boxlayout1->addLayout(vboxLayout1);
        vboxLayout1->setAlignment(Qt::AlignTop);
        vboxLayout1->setObjectName(QString::fromUtf8("vboxLayout1"));
        cbEnablePreview = new QCheckBox(bgPreview);
        cbEnablePreview->setObjectName(QString::fromUtf8("cbEnablePreview"));

        vboxLayout1->addWidget(cbEnablePreview);

        gvPreview = new QG_GraphicView(bgPreview);
        gvPreview->setObjectName(QString::fromUtf8("gvPreview"));

        vboxLayout1->addWidget(gvPreview);


        hboxLayout->addWidget(bgPreview);


        vboxLayout->addLayout(hboxLayout);

        hboxLayout1 = new QHBoxLayout();
        hboxLayout1->setSpacing(6);
        hboxLayout1->setContentsMargins(0, 0, 0, 0);
        hboxLayout1->setObjectName(QString::fromUtf8("hboxLayout1"));
        Horizontal_Spacing2 = new QSpacerItem(20, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        hboxLayout1->addItem(Horizontal_Spacing2);

        buttonOk = new QPushButton(QG_DlgHatch);
        buttonOk->setObjectName(QString::fromUtf8("buttonOk"));
        buttonOk->setAutoDefault(true);
        buttonOk->setDefault(true);

        hboxLayout1->addWidget(buttonOk);

        buttonCancel = new QPushButton(QG_DlgHatch);
        buttonCancel->setObjectName(QString::fromUtf8("buttonCancel"));
        buttonCancel->setAutoDefault(true);

        hboxLayout1->addWidget(buttonCancel);


        vboxLayout->addLayout(hboxLayout1);

        QWidget::setTabOrder(cbSolid, leScale);
        QWidget::setTabOrder(leScale, leAngle);
        QWidget::setTabOrder(leAngle, cbEnablePreview);
        QWidget::setTabOrder(cbEnablePreview, buttonOk);
        QWidget::setTabOrder(buttonOk, buttonCancel);

        retranslateUi(QG_DlgHatch);
        QObject::connect(buttonOk, SIGNAL(clicked()), QG_DlgHatch, SLOT(accept()));
        QObject::connect(buttonCancel, SIGNAL(clicked()), QG_DlgHatch, SLOT(reject()));
        QObject::connect(cbSolid, SIGNAL(toggled(bool)), cbPattern, SLOT(setDisabled(bool)));
        QObject::connect(cbSolid, SIGNAL(toggled(bool)), leScale, SLOT(setDisabled(bool)));
        QObject::connect(cbSolid, SIGNAL(toggled(bool)), lScale, SLOT(setDisabled(bool)));
        QObject::connect(cbSolid, SIGNAL(toggled(bool)), leAngle, SLOT(setDisabled(bool)));
        QObject::connect(cbSolid, SIGNAL(toggled(bool)), lAngle, SLOT(setDisabled(bool)));
        QObject::connect(cbPattern, SIGNAL(patternChanged(RS_Pattern*)), QG_DlgHatch, SLOT(updatePreview(RS_Pattern*)));
        QObject::connect(cbSolid, SIGNAL(toggled(bool)), QG_DlgHatch, SLOT(updatePreview()));
        QObject::connect(leAngle, SIGNAL(textChanged(QString)), QG_DlgHatch, SLOT(updatePreview()));
        QObject::connect(cbEnablePreview, SIGNAL(toggled(bool)), QG_DlgHatch, SLOT(updatePreview()));

        QMetaObject::connectSlotsByName(QG_DlgHatch);
    } // setupUi

    void retranslateUi(QDialog *QG_DlgHatch)
    {
        QG_DlgHatch->setWindowTitle(QApplication::translate("QG_DlgHatch", "Choose Hatch Attributes", 0, QApplication::UnicodeUTF8));
        bgParameter->setTitle(QApplication::translate("QG_DlgHatch", "Pattern", 0, QApplication::UnicodeUTF8));
        lAngle->setText(QApplication::translate("QG_DlgHatch", "Angle:", 0, QApplication::UnicodeUTF8));
        lScale->setText(QApplication::translate("QG_DlgHatch", "Scale:", 0, QApplication::UnicodeUTF8));
        cbSolid->setText(QApplication::translate("QG_DlgHatch", "Solid Fill", 0, QApplication::UnicodeUTF8));
        bgPreview->setTitle(QApplication::translate("QG_DlgHatch", "Preview", 0, QApplication::UnicodeUTF8));
        cbEnablePreview->setText(QApplication::translate("QG_DlgHatch", "Enable Preview", 0, QApplication::UnicodeUTF8));
        buttonOk->setText(QApplication::translate("QG_DlgHatch", "&OK", 0, QApplication::UnicodeUTF8));
        buttonOk->setShortcut(QApplication::translate("QG_DlgHatch", "Alt+O", 0, QApplication::UnicodeUTF8));
        buttonCancel->setText(QApplication::translate("QG_DlgHatch", "Cancel", 0, QApplication::UnicodeUTF8));
        buttonCancel->setShortcut(QString());
    } // retranslateUi


protected:
    enum IconID
    {
        image0_ID,
        unknown_ID
    };
    static QPixmap qt_get_icon(IconID id)
    {
    static const char* const image0_data[] = { 
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
        default: return QPixmap();
    } // switch
    } // icon

};

namespace Ui {
    class QG_DlgHatch: public Ui_QG_DlgHatch {};
} // namespace Ui

QT_END_NAMESPACE

class QG_DlgHatch : public QDialog, public Ui::QG_DlgHatch
{
    Q_OBJECT

public:
    QG_DlgHatch(QWidget* parent = 0, const char* name = 0, bool modal = false, Qt::WindowFlags fl = 0);
    ~QG_DlgHatch();

public slots:
    virtual void polish();
    virtual void showEvent( QShowEvent * e );
    virtual void setHatch( RS_Hatch & h, bool isNew );
    virtual void updateHatch();
    virtual void setPattern( const QString & p );
    virtual void resizeEvent( QResizeEvent * );
    virtual void updatePreview();
    virtual void updatePreview( RS_Pattern * );

protected slots:
    virtual void languageChange();

private:
    RS_EntityContainer* preview;
    bool isNew;
    RS_Pattern* pattern;
    RS_Hatch* hatch;

    void init();
    void destroy();

};

#endif // QG_DLGHATCH_H
