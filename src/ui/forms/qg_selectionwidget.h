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
#ifndef QG_SELECTIONWIDGET_H
#define QG_SELECTIONWIDGET_H

#include <qvariant.h>


#include <Qt3Support/Q3MimeSourceFactory>
#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QHeaderView>
#include <QtGui/QLabel>
#include <QtGui/QVBoxLayout>
#include <QtGui/QWidget>

QT_BEGIN_NAMESPACE

class Ui_QG_SelectionWidget
{
public:
    QVBoxLayout *vboxLayout;
    QLabel *lLabel;
    QLabel *lEntities;

    void setupUi(QWidget *QG_SelectionWidget)
    {
        if (QG_SelectionWidget->objectName().isEmpty())
            QG_SelectionWidget->setObjectName(QString::fromUtf8("QG_SelectionWidget"));
        QG_SelectionWidget->resize(124, 29);
        QSizePolicy sizePolicy(static_cast<QSizePolicy::Policy>(1), static_cast<QSizePolicy::Policy>(1));
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(QG_SelectionWidget->sizePolicy().hasHeightForWidth());
        QG_SelectionWidget->setSizePolicy(sizePolicy);
        QG_SelectionWidget->setMinimumSize(QSize(100, 27));
        QG_SelectionWidget->setMaximumSize(QSize(160, 50));
        vboxLayout = new QVBoxLayout(QG_SelectionWidget);
        vboxLayout->setSpacing(0);
        vboxLayout->setContentsMargins(0, 0, 0, 0);
        vboxLayout->setObjectName(QString::fromUtf8("vboxLayout"));
        lLabel = new QLabel(QG_SelectionWidget);
        lLabel->setObjectName(QString::fromUtf8("lLabel"));
        QFont font;
        font.setFamily(QString::fromUtf8("Helvetica"));
        font.setPointSize(7);
        lLabel->setFont(font);
        lLabel->setFrameShape(QFrame::NoFrame);
        lLabel->setFrameShadow(QFrame::Plain);
        lLabel->setWordWrap(false);

        vboxLayout->addWidget(lLabel);

        lEntities = new QLabel(QG_SelectionWidget);
        lEntities->setObjectName(QString::fromUtf8("lEntities"));
        lEntities->setFont(font);
        lEntities->setWordWrap(false);

        vboxLayout->addWidget(lEntities);


        retranslateUi(QG_SelectionWidget);

        QMetaObject::connectSlotsByName(QG_SelectionWidget);
    } // setupUi

    void retranslateUi(QWidget *QG_SelectionWidget)
    {
        QG_SelectionWidget->setWindowTitle(QApplication::translate("QG_SelectionWidget", "Selection", 0, QApplication::UnicodeUTF8));
        lLabel->setText(QApplication::translate("QG_SelectionWidget", "Selected Entities:", 0, QApplication::UnicodeUTF8));
        lEntities->setText(QApplication::translate("QG_SelectionWidget", "0", 0, QApplication::UnicodeUTF8));
    } // retranslateUi

};

namespace Ui {
    class QG_SelectionWidget: public Ui_QG_SelectionWidget {};
} // namespace Ui

QT_END_NAMESPACE

class QG_SelectionWidget : public QWidget, public Ui::QG_SelectionWidget
{
    Q_OBJECT

public:
    QG_SelectionWidget(QWidget* parent = 0, const char* name = 0, Qt::WindowFlags fl = 0);
    ~QG_SelectionWidget();

public slots:
    virtual void init();
    virtual void setNumber( int n );

protected slots:
    virtual void languageChange();

};

#endif // QG_SELECTIONWIDGET_H
