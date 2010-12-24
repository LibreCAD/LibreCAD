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
#ifndef QG_LINEPARALLELOPTIONS_H
#define QG_LINEPARALLELOPTIONS_H

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
#include "rs_actiondrawlineparallel.h"
#include "rs_line.h"
#include "rs_settings.h"

QT_BEGIN_NAMESPACE

class Ui_QG_LineParallelOptions
{
public:
    QHBoxLayout *hboxLayout;
    QLabel *lDist;
    QLineEdit *leDist;
    QLabel *lNumber;
    QSpinBox *sbNumber;
    QFrame *sep1;

    void setupUi(QWidget *QG_LineParallelOptions)
    {
        if (QG_LineParallelOptions->objectName().isEmpty())
            QG_LineParallelOptions->setObjectName(QString::fromUtf8("QG_LineParallelOptions"));
        QG_LineParallelOptions->resize(252, 22);
        QSizePolicy sizePolicy(static_cast<QSizePolicy::Policy>(4), static_cast<QSizePolicy::Policy>(5));
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(QG_LineParallelOptions->sizePolicy().hasHeightForWidth());
        QG_LineParallelOptions->setSizePolicy(sizePolicy);
        QG_LineParallelOptions->setMinimumSize(QSize(200, 22));
        QG_LineParallelOptions->setMaximumSize(QSize(280, 22));
        hboxLayout = new QHBoxLayout(QG_LineParallelOptions);
        hboxLayout->setSpacing(6);
        hboxLayout->setContentsMargins(1, 1, 1, 1);
        hboxLayout->setObjectName(QString::fromUtf8("hboxLayout"));
        lDist = new QLabel(QG_LineParallelOptions);
        lDist->setObjectName(QString::fromUtf8("lDist"));
        QSizePolicy sizePolicy1(static_cast<QSizePolicy::Policy>(1), static_cast<QSizePolicy::Policy>(0));
        sizePolicy1.setHorizontalStretch(0);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(lDist->sizePolicy().hasHeightForWidth());
        lDist->setSizePolicy(sizePolicy1);
        lDist->setWordWrap(false);

        hboxLayout->addWidget(lDist);

        leDist = new QLineEdit(QG_LineParallelOptions);
        leDist->setObjectName(QString::fromUtf8("leDist"));
        QSizePolicy sizePolicy2(static_cast<QSizePolicy::Policy>(5), static_cast<QSizePolicy::Policy>(0));
        sizePolicy2.setHorizontalStretch(0);
        sizePolicy2.setVerticalStretch(0);
        sizePolicy2.setHeightForWidth(leDist->sizePolicy().hasHeightForWidth());
        leDist->setSizePolicy(sizePolicy2);

        hboxLayout->addWidget(leDist);

        lNumber = new QLabel(QG_LineParallelOptions);
        lNumber->setObjectName(QString::fromUtf8("lNumber"));
        sizePolicy1.setHeightForWidth(lNumber->sizePolicy().hasHeightForWidth());
        lNumber->setSizePolicy(sizePolicy1);
        lNumber->setWordWrap(false);

        hboxLayout->addWidget(lNumber);

        sbNumber = new QSpinBox(QG_LineParallelOptions);
        sbNumber->setObjectName(QString::fromUtf8("sbNumber"));
        sbNumber->setMinimum(1);

        hboxLayout->addWidget(sbNumber);

        sep1 = new QFrame(QG_LineParallelOptions);
        sep1->setObjectName(QString::fromUtf8("sep1"));
        QSizePolicy sizePolicy3(static_cast<QSizePolicy::Policy>(0), static_cast<QSizePolicy::Policy>(1));
        sizePolicy3.setHorizontalStretch(0);
        sizePolicy3.setVerticalStretch(0);
        sizePolicy3.setHeightForWidth(sep1->sizePolicy().hasHeightForWidth());
        sep1->setSizePolicy(sizePolicy3);
        sep1->setFrameShape(QFrame::VLine);
        sep1->setFrameShadow(QFrame::Sunken);

        hboxLayout->addWidget(sep1);


        retranslateUi(QG_LineParallelOptions);
        QObject::connect(leDist, SIGNAL(textChanged(QString)), QG_LineParallelOptions, SLOT(updateDist(QString)));
        QObject::connect(sbNumber, SIGNAL(valueChanged(int)), QG_LineParallelOptions, SLOT(updateNumber(int)));

        QMetaObject::connectSlotsByName(QG_LineParallelOptions);
    } // setupUi

    void retranslateUi(QWidget *QG_LineParallelOptions)
    {
        QG_LineParallelOptions->setWindowTitle(QApplication::translate("QG_LineParallelOptions", "Line Parallel Options", 0, QApplication::UnicodeUTF8));
        lDist->setText(QApplication::translate("QG_LineParallelOptions", "Distance:", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        leDist->setProperty("toolTip", QVariant(QApplication::translate("QG_LineParallelOptions", "Distance to original entity", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
        lNumber->setText(QApplication::translate("QG_LineParallelOptions", "Number:", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        sbNumber->setProperty("toolTip", QVariant(QApplication::translate("QG_LineParallelOptions", "Number of parallels to create", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
    } // retranslateUi

};

namespace Ui {
    class QG_LineParallelOptions: public Ui_QG_LineParallelOptions {};
} // namespace Ui

QT_END_NAMESPACE

class QG_LineParallelOptions : public QWidget, public Ui::QG_LineParallelOptions
{
    Q_OBJECT

public:
    QG_LineParallelOptions(QWidget* parent = 0, const char* name = 0, Qt::WindowFlags fl = 0);
    ~QG_LineParallelOptions();

public slots:
    virtual void setAction( RS_ActionInterface * a, bool update );
    virtual void updateDist( const QString & d );
    virtual void updateNumber( int n );

protected:
    RS_ActionDrawLineParallel* action;

protected slots:
    virtual void languageChange();

private:
    void destroy();

};

#endif // QG_LINEPARALLELOPTIONS_H
