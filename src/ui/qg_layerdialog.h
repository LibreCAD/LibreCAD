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
#ifndef QG_LAYERDIALOG_H
#define QG_LAYERDIALOG_H

#include <qvariant.h>


#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QDialog>
#include <QtGui/QHBoxLayout>
#include <QtGui/QHeaderView>
#include <QtGui/QLabel>
#include <QtGui/QLineEdit>
#include <QtGui/QPushButton>
#include <QtGui/QSpacerItem>
#include <QtGui/QVBoxLayout>
#include "qg_widgetpen.h"
#include "rs.h"
#include "rs_layer.h"
#include "rs_layerlist.h"
#include "rs_pen.h"

QT_BEGIN_NAMESPACE

class Ui_QG_LayerDialog
{
public:
    QVBoxLayout *vboxLayout;
    QHBoxLayout *hboxLayout;
    QLabel *lName;
    QSpacerItem *spacer87;
    QLineEdit *leName;
    QG_WidgetPen *wPen;
    QHBoxLayout *hboxLayout1;
    QSpacerItem *spacer1;
    QPushButton *bOk;
    QPushButton *bCancel;

    void setupUi(QDialog *QG_LayerDialog)
    {
        if (QG_LayerDialog->objectName().isEmpty())
            QG_LayerDialog->setObjectName(QString::fromUtf8("QG_LayerDialog"));
        QG_LayerDialog->resize(253, 203);
        QG_LayerDialog->setBaseSize(QSize(0, 0));
        QG_LayerDialog->setSizeGripEnabled(false);
        vboxLayout = new QVBoxLayout(QG_LayerDialog);
        vboxLayout->setSpacing(6);
        vboxLayout->setContentsMargins(11, 11, 11, 11);
        vboxLayout->setObjectName(QString::fromUtf8("vboxLayout"));
        hboxLayout = new QHBoxLayout();
        hboxLayout->setSpacing(6);
        hboxLayout->setObjectName(QString::fromUtf8("hboxLayout"));
        lName = new QLabel(QG_LayerDialog);
        lName->setObjectName(QString::fromUtf8("lName"));
        lName->setFrameShape(QFrame::NoFrame);
        lName->setFrameShadow(QFrame::Plain);
        lName->setWordWrap(false);

        hboxLayout->addWidget(lName);

        spacer87 = new QSpacerItem(51, 21, QSizePolicy::Expanding, QSizePolicy::Minimum);

        hboxLayout->addItem(spacer87);

        leName = new QLineEdit(QG_LayerDialog);
        leName->setObjectName(QString::fromUtf8("leName"));

        hboxLayout->addWidget(leName);


        vboxLayout->addLayout(hboxLayout);

        wPen = new QG_WidgetPen(QG_LayerDialog);
        wPen->setObjectName(QString::fromUtf8("wPen"));

        vboxLayout->addWidget(wPen);

        hboxLayout1 = new QHBoxLayout();
        hboxLayout1->setSpacing(6);
        hboxLayout1->setObjectName(QString::fromUtf8("hboxLayout1"));
        spacer1 = new QSpacerItem(79, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        hboxLayout1->addItem(spacer1);

        bOk = new QPushButton(QG_LayerDialog);
        bOk->setObjectName(QString::fromUtf8("bOk"));
        bOk->setAutoDefault(true);
        bOk->setDefault(true);

        hboxLayout1->addWidget(bOk);

        bCancel = new QPushButton(QG_LayerDialog);
        bCancel->setObjectName(QString::fromUtf8("bCancel"));
        bCancel->setAutoDefault(true);

        hboxLayout1->addWidget(bCancel);


        vboxLayout->addLayout(hboxLayout1);


        retranslateUi(QG_LayerDialog);
        QObject::connect(bCancel, SIGNAL(clicked()), QG_LayerDialog, SLOT(reject()));
        QObject::connect(bOk, SIGNAL(clicked()), QG_LayerDialog, SLOT(validate()));

        QMetaObject::connectSlotsByName(QG_LayerDialog);
    } // setupUi

    void retranslateUi(QDialog *QG_LayerDialog)
    {
        QG_LayerDialog->setWindowTitle(QApplication::translate("QG_LayerDialog", "Layer Settings", 0, QApplication::UnicodeUTF8));
        lName->setText(QApplication::translate("QG_LayerDialog", "Layer Name:", 0, QApplication::UnicodeUTF8));
        leName->setText(QString());
        bOk->setText(QApplication::translate("QG_LayerDialog", "&OK", 0, QApplication::UnicodeUTF8));
        bOk->setShortcut(QApplication::translate("QG_LayerDialog", "Alt+O", 0, QApplication::UnicodeUTF8));
        bCancel->setText(QApplication::translate("QG_LayerDialog", "Cancel", 0, QApplication::UnicodeUTF8));
        bCancel->setShortcut(QApplication::translate("QG_LayerDialog", "Esc", 0, QApplication::UnicodeUTF8));
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
"0 0 0 1"};


    switch (id) {
        case image0_ID: return QPixmap((const char**)image0_data);
        default: return QPixmap();
    } // switch
    } // icon

};

namespace Ui {
    class QG_LayerDialog: public Ui_QG_LayerDialog {};
} // namespace Ui

QT_END_NAMESPACE

class QG_LayerDialog : public QDialog, public Ui::QG_LayerDialog
{
    Q_OBJECT

public:
    QG_LayerDialog(QWidget* parent = 0, const char* name = 0, bool modal = false, Qt::WindowFlags fl = 0);
    ~QG_LayerDialog();

public slots:
    virtual void setLayer( RS_Layer * l );
    virtual void updateLayer();
    virtual void validate();
    virtual void setLayerList( RS_LayerList * ll );
    virtual void setEditLayer( bool el );

protected:
    RS_Layer* layer;
    RS_LayerList* layerList;
    QString layerName;
    bool editLayer;

protected slots:
    virtual void languageChange();

private:
    void init();

};

#endif // QG_LAYERDIALOG_H
