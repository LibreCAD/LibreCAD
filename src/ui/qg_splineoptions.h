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
#ifndef QG_SPLINEOPTIONS_H
#define QG_SPLINEOPTIONS_H

#include <qvariant.h>


#include <Qt3Support/Q3MimeSourceFactory>
#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QCheckBox>
#include <QtGui/QComboBox>
#include <QtGui/QFrame>
#include <QtGui/QHBoxLayout>
#include <QtGui/QHeaderView>
#include <QtGui/QLabel>
#include <QtGui/QToolButton>
#include <QtGui/QWidget>
#include "rs_actiondrawspline.h"
#include "rs_line.h"
#include "rs_settings.h"

QT_BEGIN_NAMESPACE

class Ui_QG_SplineOptions
{
public:
    QHBoxLayout *hboxLayout;
    QLabel *lDegree;
    QComboBox *cbDegree;
    QCheckBox *cbClosed;
    QToolButton *bUndo;
    QFrame *sep1;

    void setupUi(QWidget *QG_SplineOptions)
    {
        if (QG_SplineOptions->objectName().isEmpty())
            QG_SplineOptions->setObjectName(QString::fromUtf8("QG_SplineOptions"));
        QG_SplineOptions->resize(258, 22);
        QSizePolicy sizePolicy(static_cast<QSizePolicy::Policy>(4), static_cast<QSizePolicy::Policy>(5));
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(QG_SplineOptions->sizePolicy().hasHeightForWidth());
        QG_SplineOptions->setSizePolicy(sizePolicy);
        QG_SplineOptions->setMinimumSize(QSize(200, 22));
        QG_SplineOptions->setMaximumSize(QSize(400, 22));
        hboxLayout = new QHBoxLayout(QG_SplineOptions);
        hboxLayout->setSpacing(6);
        hboxLayout->setContentsMargins(1, 1, 1, 1);
        hboxLayout->setObjectName(QString::fromUtf8("hboxLayout"));
        lDegree = new QLabel(QG_SplineOptions);
        lDegree->setObjectName(QString::fromUtf8("lDegree"));
        QSizePolicy sizePolicy1(static_cast<QSizePolicy::Policy>(5), static_cast<QSizePolicy::Policy>(5));
        sizePolicy1.setHorizontalStretch(2);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(lDegree->sizePolicy().hasHeightForWidth());
        lDegree->setSizePolicy(sizePolicy1);
        lDegree->setWordWrap(false);

        hboxLayout->addWidget(lDegree);

        cbDegree = new QComboBox(QG_SplineOptions);
        cbDegree->setObjectName(QString::fromUtf8("cbDegree"));
        QSizePolicy sizePolicy2(static_cast<QSizePolicy::Policy>(1), static_cast<QSizePolicy::Policy>(0));
        sizePolicy2.setHorizontalStretch(0);
        sizePolicy2.setVerticalStretch(0);
        sizePolicy2.setHeightForWidth(cbDegree->sizePolicy().hasHeightForWidth());
        cbDegree->setSizePolicy(sizePolicy2);

        hboxLayout->addWidget(cbDegree);

        cbClosed = new QCheckBox(QG_SplineOptions);
        cbClosed->setObjectName(QString::fromUtf8("cbClosed"));

        hboxLayout->addWidget(cbClosed);

        bUndo = new QToolButton(QG_SplineOptions);
        bUndo->setObjectName(QString::fromUtf8("bUndo"));
        QSizePolicy sizePolicy3(static_cast<QSizePolicy::Policy>(1), static_cast<QSizePolicy::Policy>(1));
        sizePolicy3.setHorizontalStretch(2);
        sizePolicy3.setVerticalStretch(0);
        sizePolicy3.setHeightForWidth(bUndo->sizePolicy().hasHeightForWidth());
        bUndo->setSizePolicy(sizePolicy3);

        hboxLayout->addWidget(bUndo);

        sep1 = new QFrame(QG_SplineOptions);
        sep1->setObjectName(QString::fromUtf8("sep1"));
        QSizePolicy sizePolicy4(static_cast<QSizePolicy::Policy>(0), static_cast<QSizePolicy::Policy>(1));
        sizePolicy4.setHorizontalStretch(0);
        sizePolicy4.setVerticalStretch(0);
        sizePolicy4.setHeightForWidth(sep1->sizePolicy().hasHeightForWidth());
        sep1->setSizePolicy(sizePolicy4);
        sep1->setFrameShape(QFrame::VLine);
        sep1->setFrameShadow(QFrame::Sunken);

        hboxLayout->addWidget(sep1);


        retranslateUi(QG_SplineOptions);
        QObject::connect(bUndo, SIGNAL(clicked()), QG_SplineOptions, SLOT(undo()));
        QObject::connect(cbDegree, SIGNAL(activated(QString)), QG_SplineOptions, SLOT(setDegree(QString)));
        QObject::connect(cbClosed, SIGNAL(toggled(bool)), QG_SplineOptions, SLOT(setClosed(bool)));

        QMetaObject::connectSlotsByName(QG_SplineOptions);
    } // setupUi

    void retranslateUi(QWidget *QG_SplineOptions)
    {
        QG_SplineOptions->setWindowTitle(QApplication::translate("QG_SplineOptions", "Spline Options", 0, QApplication::UnicodeUTF8));
        lDegree->setText(QApplication::translate("QG_SplineOptions", "Degree:", 0, QApplication::UnicodeUTF8));
        cbDegree->clear();
        cbDegree->insertItems(0, QStringList()
         << QApplication::translate("QG_SplineOptions", "1", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("QG_SplineOptions", "2", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("QG_SplineOptions", "3", 0, QApplication::UnicodeUTF8)
        );
        cbClosed->setText(QApplication::translate("QG_SplineOptions", "Closed", 0, QApplication::UnicodeUTF8));
        bUndo->setText(QApplication::translate("QG_SplineOptions", "Undo", 0, QApplication::UnicodeUTF8));
    } // retranslateUi

};

namespace Ui {
    class QG_SplineOptions: public Ui_QG_SplineOptions {};
} // namespace Ui

QT_END_NAMESPACE

class QG_SplineOptions : public QWidget, public Ui::QG_SplineOptions
{
    Q_OBJECT

public:
    QG_SplineOptions(QWidget* parent = 0, const char* name = 0, Qt::WindowFlags fl = 0);
    ~QG_SplineOptions();

public slots:
    virtual void setAction( RS_ActionInterface * a, bool update );
    virtual void setClosed( bool c );
    virtual void undo();
    virtual void setDegree( const QString & deg );

protected:
    RS_ActionDrawSpline* action;

protected slots:
    virtual void languageChange();

private:
    void destroy();

};

#endif // QG_SPLINEOPTIONS_H
