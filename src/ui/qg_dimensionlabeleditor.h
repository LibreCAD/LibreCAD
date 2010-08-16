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
#ifndef QG_DIMENSIONLABELEDITOR_H
#define QG_DIMENSIONLABELEDITOR_H

#include <qvariant.h>


#include <Qt3Support/Q3ButtonGroup>
#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QComboBox>
#include <QtGui/QHBoxLayout>
#include <QtGui/QHeaderView>
#include <QtGui/QLabel>
#include <QtGui/QLineEdit>
#include <QtGui/QToolButton>
#include <QtGui/QVBoxLayout>
#include <QtGui/QWidget>

QT_BEGIN_NAMESPACE

class Ui_QG_DimensionLabelEditor
{
public:
    QVBoxLayout *vboxLayout;
    Q3ButtonGroup *bgLabel;
    QVBoxLayout *vboxLayout1;
    QHBoxLayout *hboxLayout;
    QHBoxLayout *hboxLayout1;
    QLabel *lLabel;
    QToolButton *bDiameter;
    QLineEdit *leLabel;
    QVBoxLayout *vboxLayout2;
    QLineEdit *leTol1;
    QLineEdit *leTol2;
    QHBoxLayout *hboxLayout2;
    QLabel *textLabel1;
    QComboBox *cbSymbol;

    void setupUi(QWidget *QG_DimensionLabelEditor)
    {
        if (QG_DimensionLabelEditor->objectName().isEmpty())
            QG_DimensionLabelEditor->setObjectName(QString::fromUtf8("QG_DimensionLabelEditor"));
        QG_DimensionLabelEditor->resize(220, 105);
        QG_DimensionLabelEditor->setMinimumSize(QSize(220, 0));
        vboxLayout = new QVBoxLayout(QG_DimensionLabelEditor);
        vboxLayout->setSpacing(0);
        vboxLayout->setContentsMargins(0, 0, 0, 0);
        vboxLayout->setObjectName(QString::fromUtf8("vboxLayout"));
        bgLabel = new Q3ButtonGroup(QG_DimensionLabelEditor);
        bgLabel->setObjectName(QString::fromUtf8("bgLabel"));
        bgLabel->setColumnLayout(0, Qt::Vertical);
        bgLabel->layout()->setSpacing(0);
        bgLabel->layout()->setContentsMargins(11, 11, 11, 11);
        vboxLayout1 = new QVBoxLayout();
        QBoxLayout *boxlayout = qobject_cast<QBoxLayout *>(bgLabel->layout());
        if (boxlayout)
            boxlayout->addLayout(vboxLayout1);
        vboxLayout1->setAlignment(Qt::AlignTop);
        vboxLayout1->setObjectName(QString::fromUtf8("vboxLayout1"));
        hboxLayout = new QHBoxLayout();
        hboxLayout->setSpacing(6);
        hboxLayout->setObjectName(QString::fromUtf8("hboxLayout"));
        hboxLayout1 = new QHBoxLayout();
        hboxLayout1->setSpacing(6);
        hboxLayout1->setObjectName(QString::fromUtf8("hboxLayout1"));
        lLabel = new QLabel(bgLabel);
        lLabel->setObjectName(QString::fromUtf8("lLabel"));
        QSizePolicy sizePolicy(static_cast<QSizePolicy::Policy>(0), static_cast<QSizePolicy::Policy>(0));
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(lLabel->sizePolicy().hasHeightForWidth());
        lLabel->setSizePolicy(sizePolicy);
        lLabel->setFrameShape(QFrame::NoFrame);
        lLabel->setFrameShadow(QFrame::Plain);
        lLabel->setWordWrap(false);

        hboxLayout1->addWidget(lLabel);

        bDiameter = new QToolButton(bgLabel);
        bDiameter->setObjectName(QString::fromUtf8("bDiameter"));
        QSizePolicy sizePolicy1(static_cast<QSizePolicy::Policy>(1), static_cast<QSizePolicy::Policy>(0));
        sizePolicy1.setHorizontalStretch(0);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(bDiameter->sizePolicy().hasHeightForWidth());
        bDiameter->setSizePolicy(sizePolicy1);
        bDiameter->setIcon(qt_get_icon(image0_ID));
        bDiameter->setCheckable(true);

        hboxLayout1->addWidget(bDiameter);

        leLabel = new QLineEdit(bgLabel);
        leLabel->setObjectName(QString::fromUtf8("leLabel"));
        QSizePolicy sizePolicy2(static_cast<QSizePolicy::Policy>(5), static_cast<QSizePolicy::Policy>(0));
        sizePolicy2.setHorizontalStretch(0);
        sizePolicy2.setVerticalStretch(0);
        sizePolicy2.setHeightForWidth(leLabel->sizePolicy().hasHeightForWidth());
        leLabel->setSizePolicy(sizePolicy2);

        hboxLayout1->addWidget(leLabel);


        hboxLayout->addLayout(hboxLayout1);

        vboxLayout2 = new QVBoxLayout();
        vboxLayout2->setSpacing(0);
        vboxLayout2->setContentsMargins(0, 0, 0, 0);
        vboxLayout2->setObjectName(QString::fromUtf8("vboxLayout2"));
        leTol1 = new QLineEdit(bgLabel);
        leTol1->setObjectName(QString::fromUtf8("leTol1"));
        sizePolicy2.setHeightForWidth(leTol1->sizePolicy().hasHeightForWidth());
        leTol1->setSizePolicy(sizePolicy2);

        vboxLayout2->addWidget(leTol1);

        leTol2 = new QLineEdit(bgLabel);
        leTol2->setObjectName(QString::fromUtf8("leTol2"));
        sizePolicy2.setHeightForWidth(leTol2->sizePolicy().hasHeightForWidth());
        leTol2->setSizePolicy(sizePolicy2);

        vboxLayout2->addWidget(leTol2);


        hboxLayout->addLayout(vboxLayout2);


        vboxLayout1->addLayout(hboxLayout);

        hboxLayout2 = new QHBoxLayout();
        hboxLayout2->setSpacing(6);
        hboxLayout2->setObjectName(QString::fromUtf8("hboxLayout2"));
        textLabel1 = new QLabel(bgLabel);
        textLabel1->setObjectName(QString::fromUtf8("textLabel1"));
        textLabel1->setWordWrap(false);

        hboxLayout2->addWidget(textLabel1);

        cbSymbol = new QComboBox(bgLabel);
        cbSymbol->setObjectName(QString::fromUtf8("cbSymbol"));
        QSizePolicy sizePolicy3(static_cast<QSizePolicy::Policy>(3), static_cast<QSizePolicy::Policy>(0));
        sizePolicy3.setHorizontalStretch(0);
        sizePolicy3.setVerticalStretch(0);
        sizePolicy3.setHeightForWidth(cbSymbol->sizePolicy().hasHeightForWidth());
        cbSymbol->setSizePolicy(sizePolicy3);

        hboxLayout2->addWidget(cbSymbol);


        vboxLayout1->addLayout(hboxLayout2);


        vboxLayout->addWidget(bgLabel);


        retranslateUi(QG_DimensionLabelEditor);
        QObject::connect(cbSymbol, SIGNAL(activated(QString)), QG_DimensionLabelEditor, SLOT(insertSign(QString)));

        QMetaObject::connectSlotsByName(QG_DimensionLabelEditor);
    } // setupUi

    void retranslateUi(QWidget *QG_DimensionLabelEditor)
    {
        QG_DimensionLabelEditor->setWindowTitle(QApplication::translate("QG_DimensionLabelEditor", "Dimension Label Editor", 0, QApplication::UnicodeUTF8));
        bgLabel->setTitle(QApplication::translate("QG_DimensionLabelEditor", "Dimension Label:", 0, QApplication::UnicodeUTF8));
        lLabel->setText(QApplication::translate("QG_DimensionLabelEditor", "Label:", 0, QApplication::UnicodeUTF8));
        bDiameter->setText(QString());
        textLabel1->setText(QApplication::translate("QG_DimensionLabelEditor", "Insert:", 0, QApplication::UnicodeUTF8));
        cbSymbol->clear();
        cbSymbol->insertItems(0, QStringList()
         << QApplication::translate("QG_DimensionLabelEditor", "\303\270 (Diameter)", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("QG_DimensionLabelEditor", "\302\260 (Degree)", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("QG_DimensionLabelEditor", "\302\261 (Plus / Minus)", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("QG_DimensionLabelEditor", "\302\266 (Pi)", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("QG_DimensionLabelEditor", "\303\227 (Times)", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("QG_DimensionLabelEditor", "\303\267 (Division)", 0, QApplication::UnicodeUTF8)
        );
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
"15 15 2 1",
". c None",
"# c #000000",
"...............",
"...............",
"...............",
".....####.#....",
"....#....#.....",
"...#....#.#....",
"...#...#..#....",
"...#..#...#....",
"...#.#....#....",
"....#....#.....",
"...#.####......",
"...............",
"...............",
"...............",
"..............."};


    switch (id) {
        case image0_ID: return QPixmap((const char**)image0_data);
        default: return QPixmap();
    } // switch
    } // icon

};

namespace Ui {
    class QG_DimensionLabelEditor: public Ui_QG_DimensionLabelEditor {};
} // namespace Ui

QT_END_NAMESPACE

class QG_DimensionLabelEditor : public QWidget, public Ui::QG_DimensionLabelEditor
{
    Q_OBJECT

public:
    QG_DimensionLabelEditor(QWidget* parent = 0, const char* name = 0, Qt::WindowFlags fl = 0);
    ~QG_DimensionLabelEditor();

    virtual QString getLabel();

public slots:
    virtual void setLabel( const QString & l );
    virtual void insertSign( const QString & s );

protected slots:
    virtual void languageChange();

};

#endif // QG_DIMENSIONLABELEDITOR_H
