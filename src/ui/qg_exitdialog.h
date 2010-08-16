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
#ifndef QG_EXITDIALOG_H
#define QG_EXITDIALOG_H

#include <qvariant.h>


#include <Qt3Support/Q3MimeSourceFactory>
#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QDialog>
#include <QtGui/QGridLayout>
#include <QtGui/QHBoxLayout>
#include <QtGui/QHeaderView>
#include <QtGui/QLabel>
#include <QtGui/QPushButton>
#include <QtGui/QSpacerItem>

QT_BEGIN_NAMESPACE

class Ui_QG_ExitDialog
{
public:
    QGridLayout *gridLayout;
    QHBoxLayout *hboxLayout;
    QSpacerItem *Horizontal_Spacing2;
    QPushButton *bLeave;
    QPushButton *bSave;
    QPushButton *bSaveAs;
    QPushButton *bCancel;
    QSpacerItem *spacer2;
    QLabel *lQuestion;
    QLabel *l_icon;

    void setupUi(QDialog *QG_ExitDialog)
    {
        if (QG_ExitDialog->objectName().isEmpty())
            QG_ExitDialog->setObjectName(QString::fromUtf8("QG_ExitDialog"));
        QG_ExitDialog->resize(450, 106);
        QSizePolicy sizePolicy(static_cast<QSizePolicy::Policy>(1), static_cast<QSizePolicy::Policy>(5));
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(QG_ExitDialog->sizePolicy().hasHeightForWidth());
        QG_ExitDialog->setSizePolicy(sizePolicy);
        QG_ExitDialog->setMinimumSize(QSize(450, 0));
        QG_ExitDialog->setSizeGripEnabled(false);
        gridLayout = new QGridLayout(QG_ExitDialog);
        gridLayout->setSpacing(6);
        gridLayout->setContentsMargins(11, 11, 11, 11);
        gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
        hboxLayout = new QHBoxLayout();
        hboxLayout->setSpacing(6);
        hboxLayout->setObjectName(QString::fromUtf8("hboxLayout"));
        Horizontal_Spacing2 = new QSpacerItem(20, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        hboxLayout->addItem(Horizontal_Spacing2);

        bLeave = new QPushButton(QG_ExitDialog);
        bLeave->setObjectName(QString::fromUtf8("bLeave"));
        bLeave->setAutoDefault(true);
        bLeave->setDefault(false);

        hboxLayout->addWidget(bLeave);

        bSave = new QPushButton(QG_ExitDialog);
        bSave->setObjectName(QString::fromUtf8("bSave"));

        hboxLayout->addWidget(bSave);

        bSaveAs = new QPushButton(QG_ExitDialog);
        bSaveAs->setObjectName(QString::fromUtf8("bSaveAs"));

        hboxLayout->addWidget(bSaveAs);

        bCancel = new QPushButton(QG_ExitDialog);
        bCancel->setObjectName(QString::fromUtf8("bCancel"));
        bCancel->setAutoDefault(true);
        bCancel->setDefault(true);

        hboxLayout->addWidget(bCancel);

        spacer2 = new QSpacerItem(20, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        hboxLayout->addItem(spacer2);


        gridLayout->addLayout(hboxLayout, 1, 0, 1, 2);

        lQuestion = new QLabel(QG_ExitDialog);
        lQuestion->setObjectName(QString::fromUtf8("lQuestion"));
        QSizePolicy sizePolicy1(static_cast<QSizePolicy::Policy>(5), static_cast<QSizePolicy::Policy>(1));
        sizePolicy1.setHorizontalStretch(1);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(lQuestion->sizePolicy().hasHeightForWidth());
        lQuestion->setSizePolicy(sizePolicy1);
        lQuestion->setAlignment(Qt::AlignCenter);
        lQuestion->setWordWrap(true);

        gridLayout->addWidget(lQuestion, 0, 1, 1, 1);

        l_icon = new QLabel(QG_ExitDialog);
        l_icon->setObjectName(QString::fromUtf8("l_icon"));
        l_icon->setWordWrap(false);

        gridLayout->addWidget(l_icon, 0, 0, 1, 1);

        QWidget::setTabOrder(bCancel, bLeave);
        QWidget::setTabOrder(bLeave, bSave);
        QWidget::setTabOrder(bSave, bSaveAs);

        retranslateUi(QG_ExitDialog);
        QObject::connect(bLeave, SIGNAL(clicked()), QG_ExitDialog, SLOT(accept()));
        QObject::connect(bCancel, SIGNAL(clicked()), QG_ExitDialog, SLOT(reject()));
        QObject::connect(bSave, SIGNAL(clicked()), QG_ExitDialog, SLOT(slotSave()));
        QObject::connect(bSaveAs, SIGNAL(clicked()), QG_ExitDialog, SLOT(slotSaveAs()));

        QMetaObject::connectSlotsByName(QG_ExitDialog);
    } // setupUi

    void retranslateUi(QDialog *QG_ExitDialog)
    {
        QG_ExitDialog->setWindowTitle(QApplication::translate("QG_ExitDialog", "CADuntu", 0, QApplication::UnicodeUTF8));
        bLeave->setText(QApplication::translate("QG_ExitDialog", "C&lose", 0, QApplication::UnicodeUTF8));
        bSave->setText(QApplication::translate("QG_ExitDialog", "&Save", 0, QApplication::UnicodeUTF8));
        bSaveAs->setText(QApplication::translate("QG_ExitDialog", "Save &As..", 0, QApplication::UnicodeUTF8));
        bCancel->setText(QApplication::translate("QG_ExitDialog", "&Cancel", 0, QApplication::UnicodeUTF8));
        lQuestion->setText(QApplication::translate("QG_ExitDialog", "No Text supplied.", 0, QApplication::UnicodeUTF8));
        l_icon->setText(QString());
    } // retranslateUi

};

namespace Ui {
    class QG_ExitDialog: public Ui_QG_ExitDialog {};
} // namespace Ui

QT_END_NAMESPACE

class QG_ExitDialog : public QDialog, public Ui::QG_ExitDialog
{
    Q_OBJECT

public:
    QG_ExitDialog(QWidget* parent = 0, const char* name = 0, bool modal = false, Qt::WindowFlags fl = 0);
    ~QG_ExitDialog();

public slots:
    virtual void setText( const QString & text );
    virtual void setTitle( const QString & text );
    virtual void setForce( bool force );
    virtual void slotSaveAs();
    virtual void slotSave();

protected slots:
    virtual void languageChange();

private:
    void init();

};

#endif // QG_EXITDIALOG_H
