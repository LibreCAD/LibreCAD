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
#ifndef QG_IMAGEOPTIONSDIALOG_H
#define QG_IMAGEOPTIONSDIALOG_H

#include <qvariant.h>


#include <Qt3Support/Q3ButtonGroup>
#include <Qt3Support/Q3MimeSourceFactory>
#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
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
#include <QtGui/QVBoxLayout>
#include "rs_vector.h"

QT_BEGIN_NAMESPACE

class Ui_QG_ImageOptionsDialog
{
public:
    QVBoxLayout *vboxLayout;
    Q3ButtonGroup *bgSize;
    QGridLayout *gridLayout;
    QLabel *lWidth;
    QLabel *lHeight;
    QSpacerItem *spacer3;
    QSpacerItem *spacer4;
    QLineEdit *leHeight;
    QLineEdit *leWidth;
    QLabel *lResolution;
    QComboBox *cbResolution;
    Q3ButtonGroup *bgBackground;
    QVBoxLayout *vboxLayout1;
    QRadioButton *rbWhite;
    QRadioButton *rbBlack;
    QSpacerItem *spacer2;
    QHBoxLayout *hboxLayout;
    QSpacerItem *Horizontal_Spacing2;
    QPushButton *bOK;
    QPushButton *bCancel;

    void setupUi(QDialog *QG_ImageOptionsDialog)
    {
        if (QG_ImageOptionsDialog->objectName().isEmpty())
            QG_ImageOptionsDialog->setObjectName(QString::fromUtf8("QG_ImageOptionsDialog"));
        QG_ImageOptionsDialog->resize(365, 358);
        QG_ImageOptionsDialog->setSizeGripEnabled(true);
        vboxLayout = new QVBoxLayout(QG_ImageOptionsDialog);
        vboxLayout->setSpacing(6);
        vboxLayout->setContentsMargins(11, 11, 11, 11);
        vboxLayout->setObjectName(QString::fromUtf8("vboxLayout"));
        bgSize = new Q3ButtonGroup(QG_ImageOptionsDialog);
        bgSize->setObjectName(QString::fromUtf8("bgSize"));
        bgSize->setColumnLayout(0, Qt::Vertical);
        bgSize->layout()->setSpacing(6);
        bgSize->layout()->setContentsMargins(11, 11, 11, 11);
        gridLayout = new QGridLayout();
        QBoxLayout *boxlayout = qobject_cast<QBoxLayout *>(bgSize->layout());
        if (boxlayout)
            boxlayout->addLayout(gridLayout);
        gridLayout->setAlignment(Qt::AlignTop);
        gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
        lWidth = new QLabel(bgSize);
        lWidth->setObjectName(QString::fromUtf8("lWidth"));
        lWidth->setWordWrap(false);

        gridLayout->addWidget(lWidth, 0, 0, 1, 1);

        lHeight = new QLabel(bgSize);
        lHeight->setObjectName(QString::fromUtf8("lHeight"));
        lHeight->setWordWrap(false);

        gridLayout->addWidget(lHeight, 1, 0, 1, 1);

        spacer3 = new QSpacerItem(20, 31, QSizePolicy::Minimum, QSizePolicy::Expanding);

        gridLayout->addItem(spacer3, 3, 1, 1, 1);

        spacer4 = new QSpacerItem(20, 31, QSizePolicy::Minimum, QSizePolicy::Expanding);

        gridLayout->addItem(spacer4, 3, 0, 1, 1);

        leHeight = new QLineEdit(bgSize);
        leHeight->setObjectName(QString::fromUtf8("leHeight"));

        gridLayout->addWidget(leHeight, 1, 1, 1, 1);

        leWidth = new QLineEdit(bgSize);
        leWidth->setObjectName(QString::fromUtf8("leWidth"));

        gridLayout->addWidget(leWidth, 0, 1, 1, 1);

        lResolution = new QLabel(bgSize);
        lResolution->setObjectName(QString::fromUtf8("lResolution"));
        lResolution->setWordWrap(false);

        gridLayout->addWidget(lResolution, 2, 0, 1, 1);

        cbResolution = new QComboBox(bgSize);
        cbResolution->setObjectName(QString::fromUtf8("cbResolution"));
        cbResolution->setEditable(true);

        gridLayout->addWidget(cbResolution, 2, 1, 1, 1);


        vboxLayout->addWidget(bgSize);

        bgBackground = new Q3ButtonGroup(QG_ImageOptionsDialog);
        bgBackground->setObjectName(QString::fromUtf8("bgBackground"));
        bgBackground->setColumnLayout(0, Qt::Vertical);
        bgBackground->layout()->setSpacing(6);
        bgBackground->layout()->setContentsMargins(11, 11, 11, 11);
        vboxLayout1 = new QVBoxLayout();
        QBoxLayout *boxlayout1 = qobject_cast<QBoxLayout *>(bgBackground->layout());
        if (boxlayout1)
            boxlayout1->addLayout(vboxLayout1);
        vboxLayout1->setAlignment(Qt::AlignTop);
        vboxLayout1->setObjectName(QString::fromUtf8("vboxLayout1"));
        rbWhite = new QRadioButton(bgBackground);
        rbWhite->setObjectName(QString::fromUtf8("rbWhite"));
        rbWhite->setChecked(true);

        vboxLayout1->addWidget(rbWhite);

        rbBlack = new QRadioButton(bgBackground);
        rbBlack->setObjectName(QString::fromUtf8("rbBlack"));

        vboxLayout1->addWidget(rbBlack);

        spacer2 = new QSpacerItem(20, 51, QSizePolicy::Minimum, QSizePolicy::Expanding);

        vboxLayout1->addItem(spacer2);


        vboxLayout->addWidget(bgBackground);

        hboxLayout = new QHBoxLayout();
        hboxLayout->setSpacing(6);
        hboxLayout->setContentsMargins(0, 0, 0, 0);
        hboxLayout->setObjectName(QString::fromUtf8("hboxLayout"));
        Horizontal_Spacing2 = new QSpacerItem(20, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        hboxLayout->addItem(Horizontal_Spacing2);

        bOK = new QPushButton(QG_ImageOptionsDialog);
        bOK->setObjectName(QString::fromUtf8("bOK"));
        bOK->setAutoDefault(true);
        bOK->setDefault(true);

        hboxLayout->addWidget(bOK);

        bCancel = new QPushButton(QG_ImageOptionsDialog);
        bCancel->setObjectName(QString::fromUtf8("bCancel"));
        bCancel->setAutoDefault(true);

        hboxLayout->addWidget(bCancel);


        vboxLayout->addLayout(hboxLayout);

        QWidget::setTabOrder(leWidth, leHeight);
        QWidget::setTabOrder(leHeight, cbResolution);
        QWidget::setTabOrder(cbResolution, rbWhite);
        QWidget::setTabOrder(rbWhite, bOK);
        QWidget::setTabOrder(bOK, bCancel);

        retranslateUi(QG_ImageOptionsDialog);
        QObject::connect(bOK, SIGNAL(clicked()), QG_ImageOptionsDialog, SLOT(ok()));
        QObject::connect(bCancel, SIGNAL(clicked()), QG_ImageOptionsDialog, SLOT(reject()));
        QObject::connect(leWidth, SIGNAL(textChanged(QString)), QG_ImageOptionsDialog, SLOT(sizeChanged()));
        QObject::connect(leHeight, SIGNAL(textChanged(QString)), QG_ImageOptionsDialog, SLOT(sizeChanged()));
        QObject::connect(cbResolution, SIGNAL(textChanged(QString)), QG_ImageOptionsDialog, SLOT(resolutionChanged()));

        QMetaObject::connectSlotsByName(QG_ImageOptionsDialog);
    } // setupUi

    void retranslateUi(QDialog *QG_ImageOptionsDialog)
    {
        QG_ImageOptionsDialog->setWindowTitle(QApplication::translate("QG_ImageOptionsDialog", "Image Export Options", 0, QApplication::UnicodeUTF8));
        bgSize->setTitle(QApplication::translate("QG_ImageOptionsDialog", "Bitmap Size", 0, QApplication::UnicodeUTF8));
        lWidth->setText(QApplication::translate("QG_ImageOptionsDialog", "Width:", 0, QApplication::UnicodeUTF8));
        lHeight->setText(QApplication::translate("QG_ImageOptionsDialog", "Height:", 0, QApplication::UnicodeUTF8));
        leHeight->setText(QApplication::translate("QG_ImageOptionsDialog", "480", 0, QApplication::UnicodeUTF8));
        leWidth->setText(QApplication::translate("QG_ImageOptionsDialog", "640", 0, QApplication::UnicodeUTF8));
        lResolution->setText(QApplication::translate("QG_ImageOptionsDialog", "Resolution:", 0, QApplication::UnicodeUTF8));
        cbResolution->clear();
        cbResolution->insertItems(0, QStringList()
         << QApplication::translate("QG_ImageOptionsDialog", "auto", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("QG_ImageOptionsDialog", "1", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("QG_ImageOptionsDialog", "2", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("QG_ImageOptionsDialog", "3", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("QG_ImageOptionsDialog", "4", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("QG_ImageOptionsDialog", "5", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("QG_ImageOptionsDialog", "10", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("QG_ImageOptionsDialog", "15", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("QG_ImageOptionsDialog", "20", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("QG_ImageOptionsDialog", "25", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("QG_ImageOptionsDialog", "50", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("QG_ImageOptionsDialog", "75", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("QG_ImageOptionsDialog", "100", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("QG_ImageOptionsDialog", "150", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("QG_ImageOptionsDialog", "300", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("QG_ImageOptionsDialog", "600", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("QG_ImageOptionsDialog", "1200", 0, QApplication::UnicodeUTF8)
        );
        bgBackground->setTitle(QApplication::translate("QG_ImageOptionsDialog", "Background", 0, QApplication::UnicodeUTF8));
        rbWhite->setText(QApplication::translate("QG_ImageOptionsDialog", "White", 0, QApplication::UnicodeUTF8));
        rbBlack->setText(QApplication::translate("QG_ImageOptionsDialog", "Black", 0, QApplication::UnicodeUTF8));
        bOK->setText(QApplication::translate("QG_ImageOptionsDialog", "&OK", 0, QApplication::UnicodeUTF8));
        bOK->setShortcut(QApplication::translate("QG_ImageOptionsDialog", "Alt+O", 0, QApplication::UnicodeUTF8));
        bCancel->setText(QApplication::translate("QG_ImageOptionsDialog", "Cancel", 0, QApplication::UnicodeUTF8));
        bCancel->setShortcut(QApplication::translate("QG_ImageOptionsDialog", "Esc", 0, QApplication::UnicodeUTF8));
    } // retranslateUi

};

namespace Ui {
    class QG_ImageOptionsDialog: public Ui_QG_ImageOptionsDialog {};
} // namespace Ui

QT_END_NAMESPACE

class QG_ImageOptionsDialog : public QDialog, public Ui::QG_ImageOptionsDialog
{
    Q_OBJECT

public:
    QG_ImageOptionsDialog(QWidget* parent = 0, const char* name = 0, bool modal = false, Qt::WindowFlags fl = 0);
    ~QG_ImageOptionsDialog();

    virtual QSize getSize();
    virtual bool isBackgroundBlack();

public slots:
    virtual void setGraphicSize( const RS_Vector & s );
    virtual void ok();
    virtual void sizeChanged();
    virtual void resolutionChanged();

protected slots:
    virtual void languageChange();

private:
    RS_Vector graphicSize;
    bool updateEnabled;

    void init();

};

#endif // QG_IMAGEOPTIONSDIALOG_H
