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
#ifndef QG_BEVELOPTIONS_H
#define QG_BEVELOPTIONS_H

#include <qvariant.h>


#include <Qt3Support/Q3MimeSourceFactory>
#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QCheckBox>
#include <QtGui/QFrame>
#include <QtGui/QHBoxLayout>
#include <QtGui/QHeaderView>
#include <QtGui/QLabel>
#include <QtGui/QLineEdit>
#include <QtGui/QWidget>
#include "rs_actionmodifybevel.h"
#include "rs_line.h"
#include "rs_modification.h"
#include "rs_settings.h"

QT_BEGIN_NAMESPACE

class Ui_QG_BevelOptions
{
public:
    QHBoxLayout *hboxLayout;
    QCheckBox *cbTrim;
    QFrame *sep1_2;
    QLabel *lLength1;
    QLineEdit *leLength1;
    QLabel *lLength2;
    QLineEdit *leLength2;
    QFrame *sep1;

    void setupUi(QWidget *QG_BevelOptions)
    {
        if (QG_BevelOptions->objectName().isEmpty())
            QG_BevelOptions->setObjectName(QString::fromUtf8("QG_BevelOptions"));
        QG_BevelOptions->resize(341, 24);
        QSizePolicy sizePolicy(static_cast<QSizePolicy::Policy>(4), static_cast<QSizePolicy::Policy>(0));
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(QG_BevelOptions->sizePolicy().hasHeightForWidth());
        QG_BevelOptions->setSizePolicy(sizePolicy);
        QG_BevelOptions->setMinimumSize(QSize(341, 22));
        QG_BevelOptions->setMaximumSize(QSize(400, 32767));
        hboxLayout = new QHBoxLayout(QG_BevelOptions);
        hboxLayout->setSpacing(6);
        hboxLayout->setContentsMargins(1, 1, 1, 1);
        hboxLayout->setObjectName(QString::fromUtf8("hboxLayout"));
        cbTrim = new QCheckBox(QG_BevelOptions);
        cbTrim->setObjectName(QString::fromUtf8("cbTrim"));

        hboxLayout->addWidget(cbTrim);

        sep1_2 = new QFrame(QG_BevelOptions);
        sep1_2->setObjectName(QString::fromUtf8("sep1_2"));
        QSizePolicy sizePolicy1(static_cast<QSizePolicy::Policy>(0), static_cast<QSizePolicy::Policy>(1));
        sizePolicy1.setHorizontalStretch(0);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(sep1_2->sizePolicy().hasHeightForWidth());
        sep1_2->setSizePolicy(sizePolicy1);
        sep1_2->setFrameShape(QFrame::VLine);
        sep1_2->setFrameShadow(QFrame::Sunken);

        hboxLayout->addWidget(sep1_2);

        lLength1 = new QLabel(QG_BevelOptions);
        lLength1->setObjectName(QString::fromUtf8("lLength1"));
        lLength1->setWordWrap(false);

        hboxLayout->addWidget(lLength1);

        leLength1 = new QLineEdit(QG_BevelOptions);
        leLength1->setObjectName(QString::fromUtf8("leLength1"));

        hboxLayout->addWidget(leLength1);

        lLength2 = new QLabel(QG_BevelOptions);
        lLength2->setObjectName(QString::fromUtf8("lLength2"));
        lLength2->setWordWrap(false);

        hboxLayout->addWidget(lLength2);

        leLength2 = new QLineEdit(QG_BevelOptions);
        leLength2->setObjectName(QString::fromUtf8("leLength2"));

        hboxLayout->addWidget(leLength2);

        sep1 = new QFrame(QG_BevelOptions);
        sep1->setObjectName(QString::fromUtf8("sep1"));
        sizePolicy1.setHeightForWidth(sep1->sizePolicy().hasHeightForWidth());
        sep1->setSizePolicy(sizePolicy1);
        sep1->setFrameShape(QFrame::VLine);
        sep1->setFrameShadow(QFrame::Sunken);

        hboxLayout->addWidget(sep1);


        retranslateUi(QG_BevelOptions);
        QObject::connect(leLength1, SIGNAL(textChanged(QString)), QG_BevelOptions, SLOT(updateData()));
        QObject::connect(cbTrim, SIGNAL(toggled(bool)), QG_BevelOptions, SLOT(updateData()));
        QObject::connect(leLength2, SIGNAL(textChanged(QString)), QG_BevelOptions, SLOT(updateData()));

        QMetaObject::connectSlotsByName(QG_BevelOptions);
    } // setupUi

    void retranslateUi(QWidget *QG_BevelOptions)
    {
        QG_BevelOptions->setWindowTitle(QApplication::translate("QG_BevelOptions", "Bevel Options", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        QG_BevelOptions->setProperty("toolTip", QVariant(QString()));
#endif // QT_NO_TOOLTIP
        cbTrim->setText(QApplication::translate("QG_BevelOptions", "Trim", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        cbTrim->setProperty("toolTip", QVariant(QApplication::translate("QG_BevelOptions", "Check to trim both entities to the bevel", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
        lLength1->setText(QApplication::translate("QG_BevelOptions", "Length 1:", 0, QApplication::UnicodeUTF8));
        lLength2->setText(QApplication::translate("QG_BevelOptions", "Length 2:", 0, QApplication::UnicodeUTF8));
    } // retranslateUi

};

namespace Ui {
    class QG_BevelOptions: public Ui_QG_BevelOptions {};
} // namespace Ui

QT_END_NAMESPACE

class QG_BevelOptions : public QWidget, public Ui::QG_BevelOptions
{
    Q_OBJECT

public:
    QG_BevelOptions(QWidget* parent = 0, const char* name = 0, Qt::WindowFlags fl = 0);
    ~QG_BevelOptions();

public slots:
    virtual void setAction( RS_ActionInterface * a, bool update );
    virtual void updateData();

protected:
    RS_ActionModifyBevel* action;

protected slots:
    virtual void languageChange();

private:
    void destroy();

};

#endif // QG_BEVELOPTIONS_H
