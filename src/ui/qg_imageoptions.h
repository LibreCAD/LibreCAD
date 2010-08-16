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
#ifndef QG_IMAGEOPTIONS_H
#define QG_IMAGEOPTIONS_H

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
#include "rs_actiondrawimage.h"
#include "rs_line.h"
#include "rs_modification.h"
#include "rs_settings.h"

QT_BEGIN_NAMESPACE

class Ui_QG_ImageOptions
{
public:
    QHBoxLayout *hboxLayout;
    QLabel *lAngle;
    QLineEdit *leAngle;
    QLabel *lFactor;
    QLineEdit *leFactor;
    QFrame *sep1_2;

    void setupUi(QWidget *QG_ImageOptions)
    {
        if (QG_ImageOptions->objectName().isEmpty())
            QG_ImageOptions->setObjectName(QString::fromUtf8("QG_ImageOptions"));
        QG_ImageOptions->resize(200, 24);
        QSizePolicy sizePolicy(static_cast<QSizePolicy::Policy>(4), static_cast<QSizePolicy::Policy>(0));
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(QG_ImageOptions->sizePolicy().hasHeightForWidth());
        QG_ImageOptions->setSizePolicy(sizePolicy);
        QG_ImageOptions->setMinimumSize(QSize(200, 22));
        QG_ImageOptions->setMaximumSize(QSize(250, 32767));
        hboxLayout = new QHBoxLayout(QG_ImageOptions);
        hboxLayout->setSpacing(6);
        hboxLayout->setContentsMargins(1, 1, 1, 1);
        hboxLayout->setObjectName(QString::fromUtf8("hboxLayout"));
        lAngle = new QLabel(QG_ImageOptions);
        lAngle->setObjectName(QString::fromUtf8("lAngle"));
        lAngle->setWordWrap(false);

        hboxLayout->addWidget(lAngle);

        leAngle = new QLineEdit(QG_ImageOptions);
        leAngle->setObjectName(QString::fromUtf8("leAngle"));

        hboxLayout->addWidget(leAngle);

        lFactor = new QLabel(QG_ImageOptions);
        lFactor->setObjectName(QString::fromUtf8("lFactor"));
        lFactor->setWordWrap(false);

        hboxLayout->addWidget(lFactor);

        leFactor = new QLineEdit(QG_ImageOptions);
        leFactor->setObjectName(QString::fromUtf8("leFactor"));

        hboxLayout->addWidget(leFactor);

        sep1_2 = new QFrame(QG_ImageOptions);
        sep1_2->setObjectName(QString::fromUtf8("sep1_2"));
        QSizePolicy sizePolicy1(static_cast<QSizePolicy::Policy>(0), static_cast<QSizePolicy::Policy>(1));
        sizePolicy1.setHorizontalStretch(0);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(sep1_2->sizePolicy().hasHeightForWidth());
        sep1_2->setSizePolicy(sizePolicy1);
        sep1_2->setFrameShape(QFrame::VLine);
        sep1_2->setFrameShadow(QFrame::Sunken);

        hboxLayout->addWidget(sep1_2);


        retranslateUi(QG_ImageOptions);
        QObject::connect(leAngle, SIGNAL(textChanged(QString)), QG_ImageOptions, SLOT(updateData()));
        QObject::connect(leFactor, SIGNAL(textChanged(QString)), QG_ImageOptions, SLOT(updateData()));

        QMetaObject::connectSlotsByName(QG_ImageOptions);
    } // setupUi

    void retranslateUi(QWidget *QG_ImageOptions)
    {
        QG_ImageOptions->setWindowTitle(QApplication::translate("QG_ImageOptions", "Insert Options", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        QG_ImageOptions->setProperty("toolTip", QVariant(QString()));
#endif // QT_NO_TOOLTIP
        lAngle->setText(QApplication::translate("QG_ImageOptions", "Angle:", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        leAngle->setProperty("toolTip", QVariant(QApplication::translate("QG_ImageOptions", "Rotation Angle", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
        lFactor->setText(QApplication::translate("QG_ImageOptions", "Factor:", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        leFactor->setProperty("toolTip", QVariant(QApplication::translate("QG_ImageOptions", "Scale Factor", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
    } // retranslateUi

};

namespace Ui {
    class QG_ImageOptions: public Ui_QG_ImageOptions {};
} // namespace Ui

QT_END_NAMESPACE

class QG_ImageOptions : public QWidget, public Ui::QG_ImageOptions
{
    Q_OBJECT

public:
    QG_ImageOptions(QWidget* parent = 0, const char* name = 0, Qt::WindowFlags fl = 0);
    ~QG_ImageOptions();

public slots:
    virtual void setAction( RS_ActionInterface * a, bool update );
    virtual void updateData();

protected:
    RS_ActionDrawImage* action;

protected slots:
    virtual void languageChange();

private:
    void destroy();

};

#endif // QG_IMAGEOPTIONS_H
