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
#ifndef QG_TRIMAMOUNTOPTIONS_H
#define QG_TRIMAMOUNTOPTIONS_H

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
#include "rs_actionmodifytrimamount.h"
#include "rs_line.h"
#include "rs_settings.h"

QT_BEGIN_NAMESPACE

class Ui_QG_TrimAmountOptions
{
public:
    QHBoxLayout *hboxLayout;
    QLabel *lDist;
    QLineEdit *leDist;
    QFrame *sep1;

    void setupUi(QWidget *QG_TrimAmountOptions)
    {
        if (QG_TrimAmountOptions->objectName().isEmpty())
            QG_TrimAmountOptions->setObjectName(QString::fromUtf8("QG_TrimAmountOptions"));
        QG_TrimAmountOptions->resize(129, 24);
        QSizePolicy sizePolicy(static_cast<QSizePolicy::Policy>(4), static_cast<QSizePolicy::Policy>(5));
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(QG_TrimAmountOptions->sizePolicy().hasHeightForWidth());
        QG_TrimAmountOptions->setSizePolicy(sizePolicy);
        QG_TrimAmountOptions->setMinimumSize(QSize(128, 22));
        QG_TrimAmountOptions->setMaximumSize(QSize(170, 32767));
        hboxLayout = new QHBoxLayout(QG_TrimAmountOptions);
        hboxLayout->setSpacing(6);
        hboxLayout->setContentsMargins(1, 1, 1, 1);
        hboxLayout->setObjectName(QString::fromUtf8("hboxLayout"));
        lDist = new QLabel(QG_TrimAmountOptions);
        lDist->setObjectName(QString::fromUtf8("lDist"));
        lDist->setWordWrap(false);

        hboxLayout->addWidget(lDist);

        leDist = new QLineEdit(QG_TrimAmountOptions);
        leDist->setObjectName(QString::fromUtf8("leDist"));

        hboxLayout->addWidget(leDist);

        sep1 = new QFrame(QG_TrimAmountOptions);
        sep1->setObjectName(QString::fromUtf8("sep1"));
        QSizePolicy sizePolicy1(static_cast<QSizePolicy::Policy>(0), static_cast<QSizePolicy::Policy>(1));
        sizePolicy1.setHorizontalStretch(0);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(sep1->sizePolicy().hasHeightForWidth());
        sep1->setSizePolicy(sizePolicy1);
        sep1->setFrameShape(QFrame::VLine);
        sep1->setFrameShadow(QFrame::Sunken);

        hboxLayout->addWidget(sep1);


        retranslateUi(QG_TrimAmountOptions);
        QObject::connect(leDist, SIGNAL(textChanged(QString)), QG_TrimAmountOptions, SLOT(updateDist(QString)));

        QMetaObject::connectSlotsByName(QG_TrimAmountOptions);
    } // setupUi

    void retranslateUi(QWidget *QG_TrimAmountOptions)
    {
        QG_TrimAmountOptions->setWindowTitle(QApplication::translate("QG_TrimAmountOptions", "Trim Amount Options", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        QG_TrimAmountOptions->setProperty("toolTip", QVariant(QApplication::translate("QG_TrimAmountOptions", "Distance. Negative values for trimming, positive values for extending.", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
        lDist->setText(QApplication::translate("QG_TrimAmountOptions", "Amount:", 0, QApplication::UnicodeUTF8));
    } // retranslateUi

};

namespace Ui {
    class QG_TrimAmountOptions: public Ui_QG_TrimAmountOptions {};
} // namespace Ui

QT_END_NAMESPACE

class QG_TrimAmountOptions : public QWidget, public Ui::QG_TrimAmountOptions
{
    Q_OBJECT

public:
    QG_TrimAmountOptions(QWidget* parent = 0, const char* name = 0, Qt::WindowFlags fl = 0);
    ~QG_TrimAmountOptions();

public slots:
    virtual void setAction( RS_ActionInterface * a, bool update );
    virtual void updateDist( const QString & d );

protected:
    RS_ActionModifyTrimAmount* action;

protected slots:
    virtual void languageChange();

private:
    void destroy();

};

#endif // QG_TRIMAMOUNTOPTIONS_H
