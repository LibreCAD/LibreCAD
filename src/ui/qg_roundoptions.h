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
#ifndef QG_ROUNDOPTIONS_H
#define QG_ROUNDOPTIONS_H

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
#include "rs_actionmodifyround.h"
#include "rs_line.h"
#include "rs_modification.h"
#include "rs_settings.h"

QT_BEGIN_NAMESPACE

class Ui_QG_RoundOptions
{
public:
    QHBoxLayout *hboxLayout;
    QCheckBox *cbTrim;
    QFrame *sep1_2;
    QLabel *lRadius;
    QLineEdit *leRadius;
    QFrame *sep1;

    void setupUi(QWidget *QG_RoundOptions)
    {
        if (QG_RoundOptions->objectName().isEmpty())
            QG_RoundOptions->setObjectName(QString::fromUtf8("QG_RoundOptions"));
        QG_RoundOptions->resize(200, 24);
        QSizePolicy sizePolicy(static_cast<QSizePolicy::Policy>(0), static_cast<QSizePolicy::Policy>(0));
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(QG_RoundOptions->sizePolicy().hasHeightForWidth());
        QG_RoundOptions->setSizePolicy(sizePolicy);
        QG_RoundOptions->setMinimumSize(QSize(170, 22));
        QG_RoundOptions->setMaximumSize(QSize(200, 32767));
        hboxLayout = new QHBoxLayout(QG_RoundOptions);
        hboxLayout->setSpacing(6);
        hboxLayout->setContentsMargins(1, 1, 1, 1);
        hboxLayout->setObjectName(QString::fromUtf8("hboxLayout"));
        cbTrim = new QCheckBox(QG_RoundOptions);
        cbTrim->setObjectName(QString::fromUtf8("cbTrim"));

        hboxLayout->addWidget(cbTrim);

        sep1_2 = new QFrame(QG_RoundOptions);
        sep1_2->setObjectName(QString::fromUtf8("sep1_2"));
        QSizePolicy sizePolicy1(static_cast<QSizePolicy::Policy>(0), static_cast<QSizePolicy::Policy>(1));
        sizePolicy1.setHorizontalStretch(0);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(sep1_2->sizePolicy().hasHeightForWidth());
        sep1_2->setSizePolicy(sizePolicy1);
        sep1_2->setFrameShape(QFrame::VLine);
        sep1_2->setFrameShadow(QFrame::Sunken);

        hboxLayout->addWidget(sep1_2);

        lRadius = new QLabel(QG_RoundOptions);
        lRadius->setObjectName(QString::fromUtf8("lRadius"));
        lRadius->setWordWrap(false);

        hboxLayout->addWidget(lRadius);

        leRadius = new QLineEdit(QG_RoundOptions);
        leRadius->setObjectName(QString::fromUtf8("leRadius"));

        hboxLayout->addWidget(leRadius);

        sep1 = new QFrame(QG_RoundOptions);
        sep1->setObjectName(QString::fromUtf8("sep1"));
        sizePolicy1.setHeightForWidth(sep1->sizePolicy().hasHeightForWidth());
        sep1->setSizePolicy(sizePolicy1);
        sep1->setFrameShape(QFrame::VLine);
        sep1->setFrameShadow(QFrame::Sunken);

        hboxLayout->addWidget(sep1);


        retranslateUi(QG_RoundOptions);
        QObject::connect(leRadius, SIGNAL(textChanged(QString)), QG_RoundOptions, SLOT(updateData()));
        QObject::connect(cbTrim, SIGNAL(toggled(bool)), QG_RoundOptions, SLOT(updateData()));

        QMetaObject::connectSlotsByName(QG_RoundOptions);
    } // setupUi

    void retranslateUi(QWidget *QG_RoundOptions)
    {
        QG_RoundOptions->setWindowTitle(QApplication::translate("QG_RoundOptions", "Round Options", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        QG_RoundOptions->setProperty("toolTip", QVariant(QString()));
#endif // QT_NO_TOOLTIP
        cbTrim->setText(QApplication::translate("QG_RoundOptions", "Trim", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        cbTrim->setProperty("toolTip", QVariant(QApplication::translate("QG_RoundOptions", "Check to trim both edges to the rounding", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
        lRadius->setText(QApplication::translate("QG_RoundOptions", "Radius:", 0, QApplication::UnicodeUTF8));
    } // retranslateUi

};

namespace Ui {
    class QG_RoundOptions: public Ui_QG_RoundOptions {};
} // namespace Ui

QT_END_NAMESPACE

class QG_RoundOptions : public QWidget, public Ui::QG_RoundOptions
{
    Q_OBJECT

public:
    QG_RoundOptions(QWidget* parent = 0, const char* name = 0, Qt::WindowFlags fl = 0);
    ~QG_RoundOptions();

public slots:
    virtual void setAction( RS_ActionInterface * a, bool update );
    virtual void updateData();

protected:
    RS_ActionModifyRound* action;

protected slots:
    virtual void languageChange();

private:
    void destroy();

};

#endif // QG_ROUNDOPTIONS_H
