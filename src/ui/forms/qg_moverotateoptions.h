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
#ifndef QG_MOVEROTATEOPTIONS_H
#define QG_MOVEROTATEOPTIONS_H

#include <qvariant.h>


#include <Qt3Support/Q3MimeSourceFactory>
#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QFrame>
#include <QtGui/QHBoxLayout>
#include <QtGui/QHeaderView>
#include <QtGui/QLabel>
#include <QtGui/QLineEdit>
#include <QtGui/QWidget>
#include "rs_actionmodifymoverotate.h"
#include "rs_line.h"
#include "rs_settings.h"

QT_BEGIN_NAMESPACE

class Ui_QG_MoveRotateOptions
{
public:
    QHBoxLayout *hboxLayout;
    QLabel *lAngle;
    QLineEdit *leAngle;
    QFrame *sep1;

    void setupUi(QWidget *QG_MoveRotateOptions)
    {
        if (QG_MoveRotateOptions->objectName().isEmpty())
            QG_MoveRotateOptions->setObjectName(QString::fromUtf8("QG_MoveRotateOptions"));
        QG_MoveRotateOptions->resize(140, 22);
        QSizePolicy sizePolicy(static_cast<QSizePolicy::Policy>(0), static_cast<QSizePolicy::Policy>(0));
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(QG_MoveRotateOptions->sizePolicy().hasHeightForWidth());
        QG_MoveRotateOptions->setSizePolicy(sizePolicy);
        QG_MoveRotateOptions->setMinimumSize(QSize(128, 22));
        QG_MoveRotateOptions->setMaximumSize(QSize(140, 22));
        hboxLayout = new QHBoxLayout(QG_MoveRotateOptions);
        hboxLayout->setSpacing(6);
        hboxLayout->setContentsMargins(1, 1, 1, 1);
        hboxLayout->setObjectName(QString::fromUtf8("hboxLayout"));
        lAngle = new QLabel(QG_MoveRotateOptions);
        lAngle->setObjectName(QString::fromUtf8("lAngle"));
        lAngle->setWordWrap(false);

        hboxLayout->addWidget(lAngle);

        leAngle = new QLineEdit(QG_MoveRotateOptions);
        leAngle->setObjectName(QString::fromUtf8("leAngle"));

        hboxLayout->addWidget(leAngle);

        sep1 = new QFrame(QG_MoveRotateOptions);
        sep1->setObjectName(QString::fromUtf8("sep1"));
        QSizePolicy sizePolicy1(static_cast<QSizePolicy::Policy>(0), static_cast<QSizePolicy::Policy>(1));
        sizePolicy1.setHorizontalStretch(0);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(sep1->sizePolicy().hasHeightForWidth());
        sep1->setSizePolicy(sizePolicy1);
        sep1->setFrameShape(QFrame::VLine);
        sep1->setFrameShadow(QFrame::Sunken);

        hboxLayout->addWidget(sep1);


        retranslateUi(QG_MoveRotateOptions);
        QObject::connect(leAngle, SIGNAL(textChanged(QString)), QG_MoveRotateOptions, SLOT(updateAngle(QString)));

        QMetaObject::connectSlotsByName(QG_MoveRotateOptions);
    } // setupUi

    void retranslateUi(QWidget *QG_MoveRotateOptions)
    {
        QG_MoveRotateOptions->setWindowTitle(QApplication::translate("QG_MoveRotateOptions", "Move Rotate Options", 0, QApplication::UnicodeUTF8));
        lAngle->setText(QApplication::translate("QG_MoveRotateOptions", "Angle:", 0, QApplication::UnicodeUTF8));
    } // retranslateUi

};

namespace Ui {
    class QG_MoveRotateOptions: public Ui_QG_MoveRotateOptions {};
} // namespace Ui

QT_END_NAMESPACE

class QG_MoveRotateOptions : public QWidget, public Ui::QG_MoveRotateOptions
{
    Q_OBJECT

public:
    QG_MoveRotateOptions(QWidget* parent = 0, const char* name = 0, Qt::WindowFlags fl = 0);
    ~QG_MoveRotateOptions();

public slots:
    virtual void setAction( RS_ActionInterface * a, bool update );
    virtual void updateAngle( const QString & a );

protected:
    RS_ActionModifyMoveRotate* action;

protected slots:
    virtual void languageChange();

private:
    void destroy();

};

#endif // QG_MOVEROTATEOPTIONS_H
