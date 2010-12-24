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
#ifndef QG_DLGOPTIONSDRAWING_H
#define QG_DLGOPTIONSDRAWING_H

#include <qvariant.h>


#include <Qt3Support/Q3ButtonGroup>
#include <Qt3Support/Q3MimeSourceFactory>
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
#include <QtGui/QLineEdit>
#include <QtGui/QPushButton>
#include <QtGui/QRadioButton>
#include <QtGui/QSpacerItem>
#include <QtGui/QTabWidget>
#include <QtGui/QVBoxLayout>
#include <QtGui/QWidget>
#include "rs_filterdxf.h"
#include "rs_graphic.h"
#include "rs_math.h"
#include "rs_system.h"

QT_BEGIN_NAMESPACE

class Ui_QG_DlgOptionsDrawing
{
public:
    QVBoxLayout *vboxLayout;
    QTabWidget *tabWidget;
    QWidget *tab;
    QVBoxLayout *vboxLayout1;
    Q3ButtonGroup *buttonGroup3;
    QVBoxLayout *vboxLayout2;
    QHBoxLayout *hboxLayout;
    QComboBox *cbPaperFormat;
    QSpacerItem *spacer9;
    QHBoxLayout *hboxLayout1;
    QRadioButton *rbLandscape;
    QRadioButton *rbPortrait;
    QSpacerItem *spacer6;
    QGridLayout *gridLayout;
    QSpacerItem *spacer8;
    QLineEdit *lePaperHeight;
    QLabel *lPageHeight;
    QLabel *lPageWidth;
    QLineEdit *lePaperWidth;
    QSpacerItem *spacer7;
    QWidget *tab1;
    QVBoxLayout *vboxLayout3;
    Q3ButtonGroup *bgUnit;
    QHBoxLayout *hboxLayout2;
    QLabel *lUnit;
    QComboBox *cbUnit;
    QSpacerItem *spacer19;
    QHBoxLayout *hboxLayout3;
    QVBoxLayout *vboxLayout4;
    Q3ButtonGroup *bgLength;
    QGridLayout *gridLayout1;
    QLabel *lLengthFormat;
    QComboBox *cbLengthFormat;
    QComboBox *cbLengthPrecision;
    QLabel *lLengthPrecision;
    QSpacerItem *spacer15;
    QSpacerItem *spacer15_2;
    Q3ButtonGroup *bgLengthPreview;
    QHBoxLayout *hboxLayout4;
    QLabel *lLinear;
    QVBoxLayout *vboxLayout5;
    Q3ButtonGroup *bgAngle;
    QGridLayout *gridLayout2;
    QLabel *lAngleFormat;
    QSpacerItem *spacer15_3;
    QSpacerItem *spacer15_2_2;
    QComboBox *cbAngleFormat;
    QLabel *lAnglePrecision;
    QComboBox *cbAnglePrecision;
    Q3ButtonGroup *bgAnglePreview;
    QHBoxLayout *hboxLayout5;
    QLabel *lAngular;
    QWidget *tab2;
    QVBoxLayout *vboxLayout6;
    Q3ButtonGroup *bgGrid;
    QVBoxLayout *vboxLayout7;
    QHBoxLayout *hboxLayout6;
    QCheckBox *cbGridOn;
    QSpacerItem *spacer16;
    QGridLayout *gridLayout3;
    QSpacerItem *spacer14_2;
    QSpacerItem *spacer14;
    QLabel *lXSpacing;
    QComboBox *cbYSpacing;
    QLabel *lYSpacing;
    QComboBox *cbXSpacing;
    QSpacerItem *spacer17;
    QWidget *tab3;
    QGridLayout *gridLayout4;
    QLabel *lDimTextHeight;
    QLabel *lDimUnit1;
    QComboBox *cbDimTextHeight;
    QLabel *lDimExe;
    QComboBox *cbDimExe;
    QLabel *lDimUnit2;
    QLabel *lDimUnit3;
    QSpacerItem *spacer11;
    QSpacerItem *spacer11_2;
    QSpacerItem *spacer11_2_2;
    QLabel *lDimAsz;
    QLabel *lDimGap;
    QLabel *lDimExo;
    QComboBox *cbDimExo;
    QComboBox *cbDimGap;
    QComboBox *cbDimAsz;
    QLabel *lDimUnit4;
    QLabel *lDimUnit5;
    QWidget *tab4;
    QVBoxLayout *vboxLayout8;
    QHBoxLayout *hboxLayout7;
    QLabel *lSplineSegs;
    QComboBox *cbSplineSegs;
    QSpacerItem *spacer19_2;
    QHBoxLayout *hboxLayout8;
    QSpacerItem *Horizontal_Spacing2;
    QPushButton *buttonOk;
    QPushButton *buttonCancel;

    void setupUi(QDialog *QG_DlgOptionsDrawing)
    {
        if (QG_DlgOptionsDrawing->objectName().isEmpty())
            QG_DlgOptionsDrawing->setObjectName(QString::fromUtf8("QG_DlgOptionsDrawing"));
        QG_DlgOptionsDrawing->resize(456, 335);
        QG_DlgOptionsDrawing->setSizeGripEnabled(true);
        vboxLayout = new QVBoxLayout(QG_DlgOptionsDrawing);
        vboxLayout->setSpacing(6);
        vboxLayout->setContentsMargins(11, 11, 11, 11);
        vboxLayout->setObjectName(QString::fromUtf8("vboxLayout"));
        tabWidget = new QTabWidget(QG_DlgOptionsDrawing);
        tabWidget->setObjectName(QString::fromUtf8("tabWidget"));
        tab = new QWidget();
        tab->setObjectName(QString::fromUtf8("tab"));
        vboxLayout1 = new QVBoxLayout(tab);
        vboxLayout1->setSpacing(6);
        vboxLayout1->setContentsMargins(11, 11, 11, 11);
        vboxLayout1->setObjectName(QString::fromUtf8("vboxLayout1"));
        buttonGroup3 = new Q3ButtonGroup(tab);
        buttonGroup3->setObjectName(QString::fromUtf8("buttonGroup3"));
        buttonGroup3->setColumnLayout(0, Qt::Vertical);
        buttonGroup3->layout()->setSpacing(6);
        buttonGroup3->layout()->setContentsMargins(11, 11, 11, 11);
        vboxLayout2 = new QVBoxLayout();
        QBoxLayout *boxlayout = qobject_cast<QBoxLayout *>(buttonGroup3->layout());
        if (boxlayout)
            boxlayout->addLayout(vboxLayout2);
        vboxLayout2->setAlignment(Qt::AlignTop);
        vboxLayout2->setObjectName(QString::fromUtf8("vboxLayout2"));
        hboxLayout = new QHBoxLayout();
        hboxLayout->setSpacing(6);
        hboxLayout->setObjectName(QString::fromUtf8("hboxLayout"));
        cbPaperFormat = new QComboBox(buttonGroup3);
        cbPaperFormat->setObjectName(QString::fromUtf8("cbPaperFormat"));

        hboxLayout->addWidget(cbPaperFormat);

        spacer9 = new QSpacerItem(211, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        hboxLayout->addItem(spacer9);


        vboxLayout2->addLayout(hboxLayout);

        hboxLayout1 = new QHBoxLayout();
        hboxLayout1->setSpacing(6);
        hboxLayout1->setObjectName(QString::fromUtf8("hboxLayout1"));
        rbLandscape = new QRadioButton(buttonGroup3);
        rbLandscape->setObjectName(QString::fromUtf8("rbLandscape"));

        hboxLayout1->addWidget(rbLandscape);

        rbPortrait = new QRadioButton(buttonGroup3);
        rbPortrait->setObjectName(QString::fromUtf8("rbPortrait"));

        hboxLayout1->addWidget(rbPortrait);

        spacer6 = new QSpacerItem(231, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        hboxLayout1->addItem(spacer6);


        vboxLayout2->addLayout(hboxLayout1);

        gridLayout = new QGridLayout();
        gridLayout->setSpacing(6);
        gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
        spacer8 = new QSpacerItem(221, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        gridLayout->addItem(spacer8, 2, 2, 1, 1);

        lePaperHeight = new QLineEdit(buttonGroup3);
        lePaperHeight->setObjectName(QString::fromUtf8("lePaperHeight"));

        gridLayout->addWidget(lePaperHeight, 1, 1, 2, 1);

        lPageHeight = new QLabel(buttonGroup3);
        lPageHeight->setObjectName(QString::fromUtf8("lPageHeight"));
        lPageHeight->setWordWrap(false);

        gridLayout->addWidget(lPageHeight, 2, 0, 1, 1);

        lPageWidth = new QLabel(buttonGroup3);
        lPageWidth->setObjectName(QString::fromUtf8("lPageWidth"));
        lPageWidth->setWordWrap(false);

        gridLayout->addWidget(lPageWidth, 0, 0, 1, 1);

        lePaperWidth = new QLineEdit(buttonGroup3);
        lePaperWidth->setObjectName(QString::fromUtf8("lePaperWidth"));

        gridLayout->addWidget(lePaperWidth, 0, 1, 1, 1);

        spacer7 = new QSpacerItem(201, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        gridLayout->addItem(spacer7, 0, 2, 2, 1);


        vboxLayout2->addLayout(gridLayout);


        vboxLayout1->addWidget(buttonGroup3);

        tabWidget->addTab(tab, QString());
        tab1 = new QWidget();
        tab1->setObjectName(QString::fromUtf8("tab1"));
        vboxLayout3 = new QVBoxLayout(tab1);
        vboxLayout3->setSpacing(6);
        vboxLayout3->setContentsMargins(11, 11, 11, 11);
        vboxLayout3->setObjectName(QString::fromUtf8("vboxLayout3"));
        bgUnit = new Q3ButtonGroup(tab1);
        bgUnit->setObjectName(QString::fromUtf8("bgUnit"));
        bgUnit->setColumnLayout(0, Qt::Vertical);
        bgUnit->layout()->setSpacing(6);
        bgUnit->layout()->setContentsMargins(11, 11, 11, 11);
        hboxLayout2 = new QHBoxLayout();
        QBoxLayout *boxlayout1 = qobject_cast<QBoxLayout *>(bgUnit->layout());
        if (boxlayout1)
            boxlayout1->addLayout(hboxLayout2);
        hboxLayout2->setAlignment(Qt::AlignTop);
        hboxLayout2->setObjectName(QString::fromUtf8("hboxLayout2"));
        lUnit = new QLabel(bgUnit);
        lUnit->setObjectName(QString::fromUtf8("lUnit"));
        lUnit->setWordWrap(false);

        hboxLayout2->addWidget(lUnit);

        cbUnit = new QComboBox(bgUnit);
        cbUnit->setObjectName(QString::fromUtf8("cbUnit"));

        hboxLayout2->addWidget(cbUnit);

        spacer19 = new QSpacerItem(71, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        hboxLayout2->addItem(spacer19);


        vboxLayout3->addWidget(bgUnit);

        hboxLayout3 = new QHBoxLayout();
        hboxLayout3->setSpacing(6);
        hboxLayout3->setObjectName(QString::fromUtf8("hboxLayout3"));
        vboxLayout4 = new QVBoxLayout();
        vboxLayout4->setSpacing(6);
        vboxLayout4->setObjectName(QString::fromUtf8("vboxLayout4"));
        bgLength = new Q3ButtonGroup(tab1);
        bgLength->setObjectName(QString::fromUtf8("bgLength"));
        bgLength->setColumnLayout(0, Qt::Vertical);
        bgLength->layout()->setSpacing(6);
        bgLength->layout()->setContentsMargins(11, 11, 11, 11);
        gridLayout1 = new QGridLayout();
        QBoxLayout *boxlayout2 = qobject_cast<QBoxLayout *>(bgLength->layout());
        if (boxlayout2)
            boxlayout2->addLayout(gridLayout1);
        gridLayout1->setAlignment(Qt::AlignTop);
        gridLayout1->setObjectName(QString::fromUtf8("gridLayout1"));
        lLengthFormat = new QLabel(bgLength);
        lLengthFormat->setObjectName(QString::fromUtf8("lLengthFormat"));
        lLengthFormat->setWordWrap(false);

        gridLayout1->addWidget(lLengthFormat, 0, 0, 1, 1);

        cbLengthFormat = new QComboBox(bgLength);
        cbLengthFormat->setObjectName(QString::fromUtf8("cbLengthFormat"));

        gridLayout1->addWidget(cbLengthFormat, 0, 1, 1, 1);

        cbLengthPrecision = new QComboBox(bgLength);
        cbLengthPrecision->setObjectName(QString::fromUtf8("cbLengthPrecision"));

        gridLayout1->addWidget(cbLengthPrecision, 1, 1, 1, 1);

        lLengthPrecision = new QLabel(bgLength);
        lLengthPrecision->setObjectName(QString::fromUtf8("lLengthPrecision"));
        lLengthPrecision->setWordWrap(false);

        gridLayout1->addWidget(lLengthPrecision, 1, 0, 1, 1);

        spacer15 = new QSpacerItem(20, 31, QSizePolicy::Minimum, QSizePolicy::Expanding);

        gridLayout1->addItem(spacer15, 2, 1, 1, 1);

        spacer15_2 = new QSpacerItem(20, 31, QSizePolicy::Minimum, QSizePolicy::Expanding);

        gridLayout1->addItem(spacer15_2, 2, 0, 1, 1);


        vboxLayout4->addWidget(bgLength);

        bgLengthPreview = new Q3ButtonGroup(tab1);
        bgLengthPreview->setObjectName(QString::fromUtf8("bgLengthPreview"));
        bgLengthPreview->setColumnLayout(0, Qt::Vertical);
        bgLengthPreview->layout()->setSpacing(6);
        bgLengthPreview->layout()->setContentsMargins(11, 11, 11, 11);
        hboxLayout4 = new QHBoxLayout();
        QBoxLayout *boxlayout3 = qobject_cast<QBoxLayout *>(bgLengthPreview->layout());
        if (boxlayout3)
            boxlayout3->addLayout(hboxLayout4);
        hboxLayout4->setAlignment(Qt::AlignTop);
        hboxLayout4->setObjectName(QString::fromUtf8("hboxLayout4"));
        lLinear = new QLabel(bgLengthPreview);
        lLinear->setObjectName(QString::fromUtf8("lLinear"));
        lLinear->setWordWrap(false);

        hboxLayout4->addWidget(lLinear);


        vboxLayout4->addWidget(bgLengthPreview);


        hboxLayout3->addLayout(vboxLayout4);

        vboxLayout5 = new QVBoxLayout();
        vboxLayout5->setSpacing(6);
        vboxLayout5->setObjectName(QString::fromUtf8("vboxLayout5"));
        bgAngle = new Q3ButtonGroup(tab1);
        bgAngle->setObjectName(QString::fromUtf8("bgAngle"));
        bgAngle->setColumnLayout(0, Qt::Vertical);
        bgAngle->layout()->setSpacing(6);
        bgAngle->layout()->setContentsMargins(11, 11, 11, 11);
        gridLayout2 = new QGridLayout();
        QBoxLayout *boxlayout4 = qobject_cast<QBoxLayout *>(bgAngle->layout());
        if (boxlayout4)
            boxlayout4->addLayout(gridLayout2);
        gridLayout2->setAlignment(Qt::AlignTop);
        gridLayout2->setObjectName(QString::fromUtf8("gridLayout2"));
        lAngleFormat = new QLabel(bgAngle);
        lAngleFormat->setObjectName(QString::fromUtf8("lAngleFormat"));
        lAngleFormat->setWordWrap(false);

        gridLayout2->addWidget(lAngleFormat, 0, 0, 1, 1);

        spacer15_3 = new QSpacerItem(20, 31, QSizePolicy::Minimum, QSizePolicy::Expanding);

        gridLayout2->addItem(spacer15_3, 2, 1, 1, 1);

        spacer15_2_2 = new QSpacerItem(20, 31, QSizePolicy::Minimum, QSizePolicy::Expanding);

        gridLayout2->addItem(spacer15_2_2, 2, 0, 1, 1);

        cbAngleFormat = new QComboBox(bgAngle);
        cbAngleFormat->setObjectName(QString::fromUtf8("cbAngleFormat"));

        gridLayout2->addWidget(cbAngleFormat, 0, 1, 1, 1);

        lAnglePrecision = new QLabel(bgAngle);
        lAnglePrecision->setObjectName(QString::fromUtf8("lAnglePrecision"));
        lAnglePrecision->setWordWrap(false);

        gridLayout2->addWidget(lAnglePrecision, 1, 0, 1, 1);

        cbAnglePrecision = new QComboBox(bgAngle);
        cbAnglePrecision->setObjectName(QString::fromUtf8("cbAnglePrecision"));

        gridLayout2->addWidget(cbAnglePrecision, 1, 1, 1, 1);


        vboxLayout5->addWidget(bgAngle);

        bgAnglePreview = new Q3ButtonGroup(tab1);
        bgAnglePreview->setObjectName(QString::fromUtf8("bgAnglePreview"));
        bgAnglePreview->setColumnLayout(0, Qt::Vertical);
        bgAnglePreview->layout()->setSpacing(6);
        bgAnglePreview->layout()->setContentsMargins(11, 11, 11, 11);
        hboxLayout5 = new QHBoxLayout();
        QBoxLayout *boxlayout5 = qobject_cast<QBoxLayout *>(bgAnglePreview->layout());
        if (boxlayout5)
            boxlayout5->addLayout(hboxLayout5);
        hboxLayout5->setAlignment(Qt::AlignTop);
        hboxLayout5->setObjectName(QString::fromUtf8("hboxLayout5"));
        lAngular = new QLabel(bgAnglePreview);
        lAngular->setObjectName(QString::fromUtf8("lAngular"));
        lAngular->setWordWrap(false);

        hboxLayout5->addWidget(lAngular);


        vboxLayout5->addWidget(bgAnglePreview);


        hboxLayout3->addLayout(vboxLayout5);


        vboxLayout3->addLayout(hboxLayout3);

        tabWidget->addTab(tab1, QString());
        tab2 = new QWidget();
        tab2->setObjectName(QString::fromUtf8("tab2"));
        vboxLayout6 = new QVBoxLayout(tab2);
        vboxLayout6->setSpacing(6);
        vboxLayout6->setContentsMargins(11, 11, 11, 11);
        vboxLayout6->setObjectName(QString::fromUtf8("vboxLayout6"));
        bgGrid = new Q3ButtonGroup(tab2);
        bgGrid->setObjectName(QString::fromUtf8("bgGrid"));
        bgGrid->setColumnLayout(0, Qt::Vertical);
        bgGrid->layout()->setSpacing(6);
        bgGrid->layout()->setContentsMargins(11, 11, 11, 11);
        vboxLayout7 = new QVBoxLayout();
        QBoxLayout *boxlayout6 = qobject_cast<QBoxLayout *>(bgGrid->layout());
        if (boxlayout6)
            boxlayout6->addLayout(vboxLayout7);
        vboxLayout7->setAlignment(Qt::AlignTop);
        vboxLayout7->setObjectName(QString::fromUtf8("vboxLayout7"));
        hboxLayout6 = new QHBoxLayout();
        hboxLayout6->setSpacing(6);
        hboxLayout6->setObjectName(QString::fromUtf8("hboxLayout6"));
        cbGridOn = new QCheckBox(bgGrid);
        cbGridOn->setObjectName(QString::fromUtf8("cbGridOn"));

        hboxLayout6->addWidget(cbGridOn);

        spacer16 = new QSpacerItem(221, 21, QSizePolicy::Expanding, QSizePolicy::Minimum);

        hboxLayout6->addItem(spacer16);


        vboxLayout7->addLayout(hboxLayout6);

        gridLayout3 = new QGridLayout();
        gridLayout3->setSpacing(6);
        gridLayout3->setObjectName(QString::fromUtf8("gridLayout3"));
        spacer14_2 = new QSpacerItem(111, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        gridLayout3->addItem(spacer14_2, 1, 2, 1, 1);

        spacer14 = new QSpacerItem(111, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        gridLayout3->addItem(spacer14, 0, 2, 1, 1);

        lXSpacing = new QLabel(bgGrid);
        lXSpacing->setObjectName(QString::fromUtf8("lXSpacing"));
        lXSpacing->setWordWrap(false);

        gridLayout3->addWidget(lXSpacing, 0, 0, 1, 1);

        cbYSpacing = new QComboBox(bgGrid);
        cbYSpacing->setObjectName(QString::fromUtf8("cbYSpacing"));
        cbYSpacing->setEditable(true);

        gridLayout3->addWidget(cbYSpacing, 1, 1, 1, 1);

        lYSpacing = new QLabel(bgGrid);
        lYSpacing->setObjectName(QString::fromUtf8("lYSpacing"));
        lYSpacing->setWordWrap(false);

        gridLayout3->addWidget(lYSpacing, 1, 0, 1, 1);

        cbXSpacing = new QComboBox(bgGrid);
        cbXSpacing->setObjectName(QString::fromUtf8("cbXSpacing"));
        cbXSpacing->setEditable(true);

        gridLayout3->addWidget(cbXSpacing, 0, 1, 1, 1);


        vboxLayout7->addLayout(gridLayout3);


        vboxLayout6->addWidget(bgGrid);

        spacer17 = new QSpacerItem(20, 71, QSizePolicy::Minimum, QSizePolicy::Expanding);

        vboxLayout6->addItem(spacer17);

        tabWidget->addTab(tab2, QString());
        tab3 = new QWidget();
        tab3->setObjectName(QString::fromUtf8("tab3"));
        gridLayout4 = new QGridLayout(tab3);
        gridLayout4->setSpacing(6);
        gridLayout4->setContentsMargins(11, 11, 11, 11);
        gridLayout4->setObjectName(QString::fromUtf8("gridLayout4"));
        lDimTextHeight = new QLabel(tab3);
        lDimTextHeight->setObjectName(QString::fromUtf8("lDimTextHeight"));
        lDimTextHeight->setWordWrap(false);

        gridLayout4->addWidget(lDimTextHeight, 0, 0, 1, 1);

        lDimUnit1 = new QLabel(tab3);
        lDimUnit1->setObjectName(QString::fromUtf8("lDimUnit1"));
        lDimUnit1->setWordWrap(false);

        gridLayout4->addWidget(lDimUnit1, 0, 2, 1, 1);

        cbDimTextHeight = new QComboBox(tab3);
        cbDimTextHeight->setObjectName(QString::fromUtf8("cbDimTextHeight"));
        cbDimTextHeight->setEditable(true);

        gridLayout4->addWidget(cbDimTextHeight, 0, 1, 1, 1);

        lDimExe = new QLabel(tab3);
        lDimExe->setObjectName(QString::fromUtf8("lDimExe"));
        lDimExe->setWordWrap(false);

        gridLayout4->addWidget(lDimExe, 1, 0, 1, 1);

        cbDimExe = new QComboBox(tab3);
        cbDimExe->setObjectName(QString::fromUtf8("cbDimExe"));
        cbDimExe->setEditable(true);

        gridLayout4->addWidget(cbDimExe, 1, 1, 1, 1);

        lDimUnit2 = new QLabel(tab3);
        lDimUnit2->setObjectName(QString::fromUtf8("lDimUnit2"));
        lDimUnit2->setWordWrap(false);

        gridLayout4->addWidget(lDimUnit2, 1, 2, 1, 1);

        lDimUnit3 = new QLabel(tab3);
        lDimUnit3->setObjectName(QString::fromUtf8("lDimUnit3"));
        lDimUnit3->setWordWrap(false);

        gridLayout4->addWidget(lDimUnit3, 2, 2, 1, 1);

        spacer11 = new QSpacerItem(20, 30, QSizePolicy::Minimum, QSizePolicy::Expanding);

        gridLayout4->addItem(spacer11, 5, 0, 1, 1);

        spacer11_2 = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

        gridLayout4->addItem(spacer11_2, 5, 1, 1, 1);

        spacer11_2_2 = new QSpacerItem(20, 30, QSizePolicy::Minimum, QSizePolicy::Expanding);

        gridLayout4->addItem(spacer11_2_2, 5, 2, 1, 1);

        lDimAsz = new QLabel(tab3);
        lDimAsz->setObjectName(QString::fromUtf8("lDimAsz"));
        lDimAsz->setWordWrap(false);

        gridLayout4->addWidget(lDimAsz, 4, 0, 1, 1);

        lDimGap = new QLabel(tab3);
        lDimGap->setObjectName(QString::fromUtf8("lDimGap"));
        lDimGap->setWordWrap(false);

        gridLayout4->addWidget(lDimGap, 3, 0, 1, 1);

        lDimExo = new QLabel(tab3);
        lDimExo->setObjectName(QString::fromUtf8("lDimExo"));
        lDimExo->setWordWrap(false);

        gridLayout4->addWidget(lDimExo, 2, 0, 1, 1);

        cbDimExo = new QComboBox(tab3);
        cbDimExo->setObjectName(QString::fromUtf8("cbDimExo"));
        cbDimExo->setEditable(true);

        gridLayout4->addWidget(cbDimExo, 2, 1, 1, 1);

        cbDimGap = new QComboBox(tab3);
        cbDimGap->setObjectName(QString::fromUtf8("cbDimGap"));
        cbDimGap->setEditable(true);

        gridLayout4->addWidget(cbDimGap, 3, 1, 1, 1);

        cbDimAsz = new QComboBox(tab3);
        cbDimAsz->setObjectName(QString::fromUtf8("cbDimAsz"));
        cbDimAsz->setEditable(true);

        gridLayout4->addWidget(cbDimAsz, 4, 1, 1, 1);

        lDimUnit4 = new QLabel(tab3);
        lDimUnit4->setObjectName(QString::fromUtf8("lDimUnit4"));
        lDimUnit4->setWordWrap(false);

        gridLayout4->addWidget(lDimUnit4, 3, 2, 1, 1);

        lDimUnit5 = new QLabel(tab3);
        lDimUnit5->setObjectName(QString::fromUtf8("lDimUnit5"));
        lDimUnit5->setWordWrap(false);

        gridLayout4->addWidget(lDimUnit5, 4, 2, 1, 1);

        tabWidget->addTab(tab3, QString());
        tab4 = new QWidget();
        tab4->setObjectName(QString::fromUtf8("tab4"));
        vboxLayout8 = new QVBoxLayout(tab4);
        vboxLayout8->setSpacing(6);
        vboxLayout8->setContentsMargins(11, 11, 11, 11);
        vboxLayout8->setObjectName(QString::fromUtf8("vboxLayout8"));
        hboxLayout7 = new QHBoxLayout();
        hboxLayout7->setSpacing(6);
        hboxLayout7->setObjectName(QString::fromUtf8("hboxLayout7"));
        lSplineSegs = new QLabel(tab4);
        lSplineSegs->setObjectName(QString::fromUtf8("lSplineSegs"));
        lSplineSegs->setWordWrap(false);

        hboxLayout7->addWidget(lSplineSegs);

        cbSplineSegs = new QComboBox(tab4);
        cbSplineSegs->setObjectName(QString::fromUtf8("cbSplineSegs"));
        cbSplineSegs->setEditable(true);

        hboxLayout7->addWidget(cbSplineSegs);


        vboxLayout8->addLayout(hboxLayout7);

        spacer19_2 = new QSpacerItem(20, 111, QSizePolicy::Minimum, QSizePolicy::Expanding);

        vboxLayout8->addItem(spacer19_2);

        tabWidget->addTab(tab4, QString());

        vboxLayout->addWidget(tabWidget);

        hboxLayout8 = new QHBoxLayout();
        hboxLayout8->setSpacing(6);
        hboxLayout8->setContentsMargins(0, 0, 0, 0);
        hboxLayout8->setObjectName(QString::fromUtf8("hboxLayout8"));
        Horizontal_Spacing2 = new QSpacerItem(20, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        hboxLayout8->addItem(Horizontal_Spacing2);

        buttonOk = new QPushButton(QG_DlgOptionsDrawing);
        buttonOk->setObjectName(QString::fromUtf8("buttonOk"));
        buttonOk->setAutoDefault(true);
        buttonOk->setDefault(true);

        hboxLayout8->addWidget(buttonOk);

        buttonCancel = new QPushButton(QG_DlgOptionsDrawing);
        buttonCancel->setObjectName(QString::fromUtf8("buttonCancel"));
        buttonCancel->setAutoDefault(true);

        hboxLayout8->addWidget(buttonCancel);


        vboxLayout->addLayout(hboxLayout8);

#ifndef QT_NO_SHORTCUT
        lPageHeight->setBuddy(lePaperHeight);
        lPageWidth->setBuddy(lePaperWidth);
        lUnit->setBuddy(cbUnit);
        lLengthFormat->setBuddy(cbLengthFormat);
        lLengthPrecision->setBuddy(cbLengthPrecision);
        lAngleFormat->setBuddy(cbAngleFormat);
        lAnglePrecision->setBuddy(cbAnglePrecision);
#endif // QT_NO_SHORTCUT
        QWidget::setTabOrder(tabWidget, cbPaperFormat);
        QWidget::setTabOrder(cbPaperFormat, rbLandscape);
        QWidget::setTabOrder(rbLandscape, rbPortrait);
        QWidget::setTabOrder(rbPortrait, lePaperWidth);
        QWidget::setTabOrder(lePaperWidth, lePaperHeight);
        QWidget::setTabOrder(lePaperHeight, buttonOk);
        QWidget::setTabOrder(buttonOk, buttonCancel);
        QWidget::setTabOrder(buttonCancel, cbUnit);
        QWidget::setTabOrder(cbUnit, cbLengthFormat);
        QWidget::setTabOrder(cbLengthFormat, cbLengthPrecision);
        QWidget::setTabOrder(cbLengthPrecision, cbAngleFormat);
        QWidget::setTabOrder(cbAngleFormat, cbAnglePrecision);
        QWidget::setTabOrder(cbAnglePrecision, cbGridOn);
        QWidget::setTabOrder(cbGridOn, cbXSpacing);
        QWidget::setTabOrder(cbXSpacing, cbYSpacing);
        QWidget::setTabOrder(cbYSpacing, cbDimTextHeight);
        QWidget::setTabOrder(cbDimTextHeight, cbDimExe);
        QWidget::setTabOrder(cbDimExe, cbDimExo);
        QWidget::setTabOrder(cbDimExo, cbDimGap);
        QWidget::setTabOrder(cbDimGap, cbDimAsz);

        retranslateUi(QG_DlgOptionsDrawing);
        QObject::connect(buttonOk, SIGNAL(clicked()), QG_DlgOptionsDrawing, SLOT(validate()));
        QObject::connect(buttonCancel, SIGNAL(clicked()), QG_DlgOptionsDrawing, SLOT(reject()));
        QObject::connect(cbLengthFormat, SIGNAL(activated(int)), QG_DlgOptionsDrawing, SLOT(updateLengthPrecision()));
        QObject::connect(cbAngleFormat, SIGNAL(activated(int)), QG_DlgOptionsDrawing, SLOT(updateAnglePrecision()));
        QObject::connect(cbUnit, SIGNAL(activated(int)), QG_DlgOptionsDrawing, SLOT(updatePreview()));
        QObject::connect(cbAngleFormat, SIGNAL(activated(int)), QG_DlgOptionsDrawing, SLOT(updatePreview()));
        QObject::connect(cbLengthFormat, SIGNAL(activated(QString)), QG_DlgOptionsDrawing, SLOT(updatePreview()));
        QObject::connect(cbAnglePrecision, SIGNAL(activated(int)), QG_DlgOptionsDrawing, SLOT(updatePreview()));
        QObject::connect(cbLengthPrecision, SIGNAL(activated(int)), QG_DlgOptionsDrawing, SLOT(updatePreview()));
        QObject::connect(cbPaperFormat, SIGNAL(activated(int)), QG_DlgOptionsDrawing, SLOT(updatePaperSize()));
        QObject::connect(cbUnit, SIGNAL(activated(int)), QG_DlgOptionsDrawing, SLOT(updateUnitLabels()));

        QMetaObject::connectSlotsByName(QG_DlgOptionsDrawing);
    } // setupUi

    void retranslateUi(QDialog *QG_DlgOptionsDrawing)
    {
        QG_DlgOptionsDrawing->setWindowTitle(QApplication::translate("QG_DlgOptionsDrawing", "Drawing Preferences", 0, QApplication::UnicodeUTF8));
        buttonGroup3->setTitle(QApplication::translate("QG_DlgOptionsDrawing", "Paper Format", 0, QApplication::UnicodeUTF8));
        rbLandscape->setText(QApplication::translate("QG_DlgOptionsDrawing", "&Landscape", 0, QApplication::UnicodeUTF8));
        rbPortrait->setText(QApplication::translate("QG_DlgOptionsDrawing", "P&ortrait", 0, QApplication::UnicodeUTF8));
        lPageHeight->setText(QApplication::translate("QG_DlgOptionsDrawing", "Paper &Height:", 0, QApplication::UnicodeUTF8));
        lPageWidth->setText(QApplication::translate("QG_DlgOptionsDrawing", "Paper &Width:", 0, QApplication::UnicodeUTF8));
        tabWidget->setTabText(tabWidget->indexOf(tab), QApplication::translate("QG_DlgOptionsDrawing", "&Paper", 0, QApplication::UnicodeUTF8));
        bgUnit->setTitle(QApplication::translate("QG_DlgOptionsDrawing", "Main Unit", 0, QApplication::UnicodeUTF8));
        lUnit->setText(QApplication::translate("QG_DlgOptionsDrawing", "&Main drawing unit:", 0, QApplication::UnicodeUTF8));
        bgLength->setTitle(QApplication::translate("QG_DlgOptionsDrawing", "Length", 0, QApplication::UnicodeUTF8));
        lLengthFormat->setText(QApplication::translate("QG_DlgOptionsDrawing", "&Format:", 0, QApplication::UnicodeUTF8));
        lLengthPrecision->setText(QApplication::translate("QG_DlgOptionsDrawing", "P&recision:", 0, QApplication::UnicodeUTF8));
        bgLengthPreview->setTitle(QApplication::translate("QG_DlgOptionsDrawing", "Preview", 0, QApplication::UnicodeUTF8));
        lLinear->setText(QApplication::translate("QG_DlgOptionsDrawing", "linear", 0, QApplication::UnicodeUTF8));
        bgAngle->setTitle(QApplication::translate("QG_DlgOptionsDrawing", "Angle", 0, QApplication::UnicodeUTF8));
        lAngleFormat->setText(QApplication::translate("QG_DlgOptionsDrawing", "F&ormat:", 0, QApplication::UnicodeUTF8));
        lAnglePrecision->setText(QApplication::translate("QG_DlgOptionsDrawing", "Pre&cision:", 0, QApplication::UnicodeUTF8));
        bgAnglePreview->setTitle(QApplication::translate("QG_DlgOptionsDrawing", "Preview", 0, QApplication::UnicodeUTF8));
        lAngular->setText(QApplication::translate("QG_DlgOptionsDrawing", "angular", 0, QApplication::UnicodeUTF8));
        tabWidget->setTabText(tabWidget->indexOf(tab1), QApplication::translate("QG_DlgOptionsDrawing", "&Units", 0, QApplication::UnicodeUTF8));
        bgGrid->setTitle(QApplication::translate("QG_DlgOptionsDrawing", "Grid Settings", 0, QApplication::UnicodeUTF8));
        cbGridOn->setText(QApplication::translate("QG_DlgOptionsDrawing", "Show Grid", 0, QApplication::UnicodeUTF8));
        lXSpacing->setText(QApplication::translate("QG_DlgOptionsDrawing", "X Spacing:", 0, QApplication::UnicodeUTF8));
        cbYSpacing->clear();
        cbYSpacing->insertItems(0, QStringList()
         << QApplication::translate("QG_DlgOptionsDrawing", "auto", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("QG_DlgOptionsDrawing", "0.01", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("QG_DlgOptionsDrawing", "0.1", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("QG_DlgOptionsDrawing", "1", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("QG_DlgOptionsDrawing", "10", 0, QApplication::UnicodeUTF8)
        );
        lYSpacing->setText(QApplication::translate("QG_DlgOptionsDrawing", "Y Spacing:", 0, QApplication::UnicodeUTF8));
        cbXSpacing->clear();
        cbXSpacing->insertItems(0, QStringList()
         << QApplication::translate("QG_DlgOptionsDrawing", "auto", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("QG_DlgOptionsDrawing", "0.01", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("QG_DlgOptionsDrawing", "0.1", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("QG_DlgOptionsDrawing", "1", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("QG_DlgOptionsDrawing", "10", 0, QApplication::UnicodeUTF8)
        );
        tabWidget->setTabText(tabWidget->indexOf(tab2), QApplication::translate("QG_DlgOptionsDrawing", "&Grid", 0, QApplication::UnicodeUTF8));
        lDimTextHeight->setText(QApplication::translate("QG_DlgOptionsDrawing", "Text Height:", 0, QApplication::UnicodeUTF8));
        lDimUnit1->setText(QApplication::translate("QG_DlgOptionsDrawing", "units", 0, QApplication::UnicodeUTF8));
        cbDimTextHeight->clear();
        cbDimTextHeight->insertItems(0, QStringList()
         << QApplication::translate("QG_DlgOptionsDrawing", "1", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("QG_DlgOptionsDrawing", "2", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("QG_DlgOptionsDrawing", "5", 0, QApplication::UnicodeUTF8)
        );
        lDimExe->setText(QApplication::translate("QG_DlgOptionsDrawing", "Extension line extension:", 0, QApplication::UnicodeUTF8));
        cbDimExe->clear();
        cbDimExe->insertItems(0, QStringList()
         << QApplication::translate("QG_DlgOptionsDrawing", "1", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("QG_DlgOptionsDrawing", "2", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("QG_DlgOptionsDrawing", "5", 0, QApplication::UnicodeUTF8)
        );
        lDimUnit2->setText(QApplication::translate("QG_DlgOptionsDrawing", "units", 0, QApplication::UnicodeUTF8));
        lDimUnit3->setText(QApplication::translate("QG_DlgOptionsDrawing", "units", 0, QApplication::UnicodeUTF8));
        lDimAsz->setText(QApplication::translate("QG_DlgOptionsDrawing", "Arrow size:", 0, QApplication::UnicodeUTF8));
        lDimGap->setText(QApplication::translate("QG_DlgOptionsDrawing", "Dimension line gap:", 0, QApplication::UnicodeUTF8));
        lDimExo->setText(QApplication::translate("QG_DlgOptionsDrawing", "Extension line offset:", 0, QApplication::UnicodeUTF8));
        cbDimExo->clear();
        cbDimExo->insertItems(0, QStringList()
         << QApplication::translate("QG_DlgOptionsDrawing", "1", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("QG_DlgOptionsDrawing", "2", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("QG_DlgOptionsDrawing", "5", 0, QApplication::UnicodeUTF8)
        );
        cbDimGap->clear();
        cbDimGap->insertItems(0, QStringList()
         << QApplication::translate("QG_DlgOptionsDrawing", "1", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("QG_DlgOptionsDrawing", "2", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("QG_DlgOptionsDrawing", "5", 0, QApplication::UnicodeUTF8)
        );
        cbDimAsz->clear();
        cbDimAsz->insertItems(0, QStringList()
         << QApplication::translate("QG_DlgOptionsDrawing", "1", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("QG_DlgOptionsDrawing", "2", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("QG_DlgOptionsDrawing", "5", 0, QApplication::UnicodeUTF8)
        );
        lDimUnit4->setText(QApplication::translate("QG_DlgOptionsDrawing", "units", 0, QApplication::UnicodeUTF8));
        lDimUnit5->setText(QApplication::translate("QG_DlgOptionsDrawing", "units", 0, QApplication::UnicodeUTF8));
        tabWidget->setTabText(tabWidget->indexOf(tab3), QApplication::translate("QG_DlgOptionsDrawing", "&Dimensions", 0, QApplication::UnicodeUTF8));
        lSplineSegs->setText(QApplication::translate("QG_DlgOptionsDrawing", "Number of line segments per spline patch:", 0, QApplication::UnicodeUTF8));
        cbSplineSegs->clear();
        cbSplineSegs->insertItems(0, QStringList()
         << QApplication::translate("QG_DlgOptionsDrawing", "2", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("QG_DlgOptionsDrawing", "4", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("QG_DlgOptionsDrawing", "8", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("QG_DlgOptionsDrawing", "16", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("QG_DlgOptionsDrawing", "32", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("QG_DlgOptionsDrawing", "64", 0, QApplication::UnicodeUTF8)
        );
        tabWidget->setTabText(tabWidget->indexOf(tab4), QApplication::translate("QG_DlgOptionsDrawing", "Splines", 0, QApplication::UnicodeUTF8));
        buttonOk->setText(QApplication::translate("QG_DlgOptionsDrawing", "&OK", 0, QApplication::UnicodeUTF8));
        buttonOk->setShortcut(QApplication::translate("QG_DlgOptionsDrawing", "Alt+O", 0, QApplication::UnicodeUTF8));
        buttonCancel->setText(QApplication::translate("QG_DlgOptionsDrawing", "Cancel", 0, QApplication::UnicodeUTF8));
        buttonCancel->setShortcut(QApplication::translate("QG_DlgOptionsDrawing", "Esc", 0, QApplication::UnicodeUTF8));
    } // retranslateUi

};

namespace Ui {
    class QG_DlgOptionsDrawing: public Ui_QG_DlgOptionsDrawing {};
} // namespace Ui

QT_END_NAMESPACE

class QG_DlgOptionsDrawing : public QDialog, public Ui::QG_DlgOptionsDrawing
{
    Q_OBJECT

public:
    QG_DlgOptionsDrawing(QWidget* parent = 0, const char* name = 0, bool modal = false, Qt::WindowFlags fl = 0);
    ~QG_DlgOptionsDrawing();

public slots:
    virtual void setGraphic( RS_Graphic * g );
    virtual void validate();
    virtual void updateLengthPrecision();
    virtual void updateAnglePrecision();
    virtual void updatePreview();
    virtual void updatePaperSize();
    virtual void updateUnitLabels();

protected slots:
    virtual void languageChange();

private:
    QStringList listPrec1;
    RS_Graphic* graphic;

    void init();

};

#endif // QG_DLGOPTIONSDRAWING_H
