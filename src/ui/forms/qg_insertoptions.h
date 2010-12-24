/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2010 R. van Twisk (librecad@rvt.dds.nl)
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
#ifndef QG_INSERTOPTIONS_H
#define QG_INSERTOPTIONS_H

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
#include <QtGui/QSpinBox>
#include <QtGui/QWidget>
#include "rs_actionblocksinsert.h"
#include "rs_line.h"
#include "rs_modification.h"
#include "rs_settings.h"

QT_BEGIN_NAMESPACE

class Ui_QG_InsertOptions
{
public:
    QHBoxLayout *hboxLayout;
    QLabel *lAngle;
    QLineEdit *leAngle;
    QLabel *lFactor;
    QLineEdit *leFactor;
    QFrame *sep1_2;
    QLabel *lArray;
    QSpinBox *sbColumns;
    QSpinBox *sbRows;
    QLabel *lSpacing;
    QLineEdit *leColumnSpacing;
    QLineEdit *leRowSpacing;
    QFrame *sep1;

    void setupUi(QWidget *QG_InsertOptions)
    {
        if (QG_InsertOptions->objectName().isEmpty())
            QG_InsertOptions->setObjectName(QString::fromUtf8("QG_InsertOptions"));
        QG_InsertOptions->resize(550, 24);
        QSizePolicy sizePolicy(static_cast<QSizePolicy::Policy>(4), static_cast<QSizePolicy::Policy>(0));
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(QG_InsertOptions->sizePolicy().hasHeightForWidth());
        QG_InsertOptions->setSizePolicy(sizePolicy);
        QG_InsertOptions->setMinimumSize(QSize(550, 22));
        QG_InsertOptions->setMaximumSize(QSize(600, 32767));
        hboxLayout = new QHBoxLayout(QG_InsertOptions);
        hboxLayout->setSpacing(6);
        hboxLayout->setContentsMargins(1, 1, 1, 1);
        hboxLayout->setObjectName(QString::fromUtf8("hboxLayout"));
        lAngle = new QLabel(QG_InsertOptions);
        lAngle->setObjectName(QString::fromUtf8("lAngle"));
        lAngle->setWordWrap(false);

        hboxLayout->addWidget(lAngle);

        leAngle = new QLineEdit(QG_InsertOptions);
        leAngle->setObjectName(QString::fromUtf8("leAngle"));

        hboxLayout->addWidget(leAngle);

        lFactor = new QLabel(QG_InsertOptions);
        lFactor->setObjectName(QString::fromUtf8("lFactor"));
        lFactor->setWordWrap(false);

        hboxLayout->addWidget(lFactor);

        leFactor = new QLineEdit(QG_InsertOptions);
        leFactor->setObjectName(QString::fromUtf8("leFactor"));

        hboxLayout->addWidget(leFactor);

        sep1_2 = new QFrame(QG_InsertOptions);
        sep1_2->setObjectName(QString::fromUtf8("sep1_2"));
        QSizePolicy sizePolicy1(static_cast<QSizePolicy::Policy>(0), static_cast<QSizePolicy::Policy>(1));
        sizePolicy1.setHorizontalStretch(0);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(sep1_2->sizePolicy().hasHeightForWidth());
        sep1_2->setSizePolicy(sizePolicy1);
        sep1_2->setFrameShape(QFrame::VLine);
        sep1_2->setFrameShadow(QFrame::Sunken);

        hboxLayout->addWidget(sep1_2);

        lArray = new QLabel(QG_InsertOptions);
        lArray->setObjectName(QString::fromUtf8("lArray"));
        lArray->setWordWrap(false);

        hboxLayout->addWidget(lArray);

        sbColumns = new QSpinBox(QG_InsertOptions);
        sbColumns->setObjectName(QString::fromUtf8("sbColumns"));
        sbColumns->setMaximum(10000);
        sbColumns->setMinimum(1);

        hboxLayout->addWidget(sbColumns);

        sbRows = new QSpinBox(QG_InsertOptions);
        sbRows->setObjectName(QString::fromUtf8("sbRows"));
        sbRows->setMaximum(10000);
        sbRows->setMinimum(1);

        hboxLayout->addWidget(sbRows);

        lSpacing = new QLabel(QG_InsertOptions);
        lSpacing->setObjectName(QString::fromUtf8("lSpacing"));
        lSpacing->setWordWrap(false);

        hboxLayout->addWidget(lSpacing);

        leColumnSpacing = new QLineEdit(QG_InsertOptions);
        leColumnSpacing->setObjectName(QString::fromUtf8("leColumnSpacing"));

        hboxLayout->addWidget(leColumnSpacing);

        leRowSpacing = new QLineEdit(QG_InsertOptions);
        leRowSpacing->setObjectName(QString::fromUtf8("leRowSpacing"));

        hboxLayout->addWidget(leRowSpacing);

        sep1 = new QFrame(QG_InsertOptions);
        sep1->setObjectName(QString::fromUtf8("sep1"));
        sizePolicy1.setHeightForWidth(sep1->sizePolicy().hasHeightForWidth());
        sep1->setSizePolicy(sizePolicy1);
        sep1->setFrameShape(QFrame::VLine);
        sep1->setFrameShadow(QFrame::Sunken);

        hboxLayout->addWidget(sep1);


        retranslateUi(QG_InsertOptions);
        QObject::connect(leAngle, SIGNAL(textChanged(QString)), QG_InsertOptions, SLOT(updateData()));
        QObject::connect(leFactor, SIGNAL(textChanged(QString)), QG_InsertOptions, SLOT(updateData()));
        QObject::connect(sbColumns, SIGNAL(valueChanged(int)), QG_InsertOptions, SLOT(updateData()));
        QObject::connect(sbRows, SIGNAL(valueChanged(int)), QG_InsertOptions, SLOT(updateData()));
        QObject::connect(leColumnSpacing, SIGNAL(textChanged(QString)), QG_InsertOptions, SLOT(updateData()));
        QObject::connect(leRowSpacing, SIGNAL(textChanged(QString)), QG_InsertOptions, SLOT(updateData()));

        QMetaObject::connectSlotsByName(QG_InsertOptions);
    } // setupUi

    void retranslateUi(QWidget *QG_InsertOptions)
    {
        QG_InsertOptions->setWindowTitle(QApplication::translate("QG_InsertOptions", "Insert Options", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        QG_InsertOptions->setProperty("toolTip", QVariant(QString()));
#endif // QT_NO_TOOLTIP
        lAngle->setText(QApplication::translate("QG_InsertOptions", "Angle:", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        leAngle->setProperty("toolTip", QVariant(QApplication::translate("QG_InsertOptions", "Rotation Angle", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
        lFactor->setText(QApplication::translate("QG_InsertOptions", "Factor:", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        leFactor->setProperty("toolTip", QVariant(QApplication::translate("QG_InsertOptions", "Scale Factor", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
        lArray->setText(QApplication::translate("QG_InsertOptions", "Array:", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        sbColumns->setProperty("toolTip", QVariant(QApplication::translate("QG_InsertOptions", "Number of Columns", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_TOOLTIP
        sbRows->setProperty("toolTip", QVariant(QApplication::translate("QG_InsertOptions", "Number of Rows", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
        lSpacing->setText(QApplication::translate("QG_InsertOptions", "Spacing:", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        leColumnSpacing->setProperty("toolTip", QVariant(QApplication::translate("QG_InsertOptions", "Column Spacing", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_TOOLTIP
        leRowSpacing->setProperty("toolTip", QVariant(QApplication::translate("QG_InsertOptions", "Row Spacing", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
    } // retranslateUi

};

namespace Ui {
    class QG_InsertOptions: public Ui_QG_InsertOptions {};
} // namespace Ui

QT_END_NAMESPACE

class QG_InsertOptions : public QWidget, public Ui::QG_InsertOptions
{
    Q_OBJECT

public:
    QG_InsertOptions(QWidget* parent = 0, const char* name = 0, Qt::WindowFlags fl = 0);
    ~QG_InsertOptions();

public slots:
    virtual void setAction( RS_ActionInterface * a, bool update );
    virtual void updateData();

protected:
    RS_ActionBlocksInsert* action;

protected slots:
    virtual void languageChange();

private:
    void destroy();

};

#endif // QG_INSERTOPTIONS_H
