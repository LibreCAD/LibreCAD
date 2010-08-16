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
#ifndef QG_ARCTANGENTIALOPTIONS_H
#define QG_ARCTANGENTIALOPTIONS_H

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
#include "rs_actiondrawarctangential.h"
#include "rs_arc.h"

QT_BEGIN_NAMESPACE

class Ui_QG_ArcTangentialOptions
{
public:
    QHBoxLayout *hboxLayout;
    QLabel *lRadius;
    QLineEdit *leRadius;
    QFrame *sep1;

    void setupUi(QWidget *QG_ArcTangentialOptions)
    {
        if (QG_ArcTangentialOptions->objectName().isEmpty())
            QG_ArcTangentialOptions->setObjectName(QString::fromUtf8("QG_ArcTangentialOptions"));
        QG_ArcTangentialOptions->resize(160, 24);
        QSizePolicy sizePolicy(static_cast<QSizePolicy::Policy>(0), static_cast<QSizePolicy::Policy>(0));
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(QG_ArcTangentialOptions->sizePolicy().hasHeightForWidth());
        QG_ArcTangentialOptions->setSizePolicy(sizePolicy);
        QG_ArcTangentialOptions->setMinimumSize(QSize(160, 22));
        hboxLayout = new QHBoxLayout(QG_ArcTangentialOptions);
        hboxLayout->setSpacing(6);
        hboxLayout->setContentsMargins(0, 0, 0, 0);
        hboxLayout->setObjectName(QString::fromUtf8("hboxLayout"));
        lRadius = new QLabel(QG_ArcTangentialOptions);
        lRadius->setObjectName(QString::fromUtf8("lRadius"));
        lRadius->setWordWrap(false);

        hboxLayout->addWidget(lRadius);

        leRadius = new QLineEdit(QG_ArcTangentialOptions);
        leRadius->setObjectName(QString::fromUtf8("leRadius"));
        QSizePolicy sizePolicy1(static_cast<QSizePolicy::Policy>(13), static_cast<QSizePolicy::Policy>(0));
        sizePolicy1.setHorizontalStretch(0);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(leRadius->sizePolicy().hasHeightForWidth());
        leRadius->setSizePolicy(sizePolicy1);

        hboxLayout->addWidget(leRadius);

        sep1 = new QFrame(QG_ArcTangentialOptions);
        sep1->setObjectName(QString::fromUtf8("sep1"));
        QSizePolicy sizePolicy2(static_cast<QSizePolicy::Policy>(0), static_cast<QSizePolicy::Policy>(1));
        sizePolicy2.setHorizontalStretch(0);
        sizePolicy2.setVerticalStretch(0);
        sizePolicy2.setHeightForWidth(sep1->sizePolicy().hasHeightForWidth());
        sep1->setSizePolicy(sizePolicy2);
        sep1->setFrameShape(QFrame::VLine);
        sep1->setFrameShadow(QFrame::Sunken);

        hboxLayout->addWidget(sep1);


        retranslateUi(QG_ArcTangentialOptions);
        QObject::connect(leRadius, SIGNAL(textChanged(QString)), QG_ArcTangentialOptions, SLOT(updateRadius(QString)));

        QMetaObject::connectSlotsByName(QG_ArcTangentialOptions);
    } // setupUi

    void retranslateUi(QWidget *QG_ArcTangentialOptions)
    {
        QG_ArcTangentialOptions->setWindowTitle(QApplication::translate("QG_ArcTangentialOptions", "Tangential Arc Options", 0, QApplication::UnicodeUTF8));
        lRadius->setText(QApplication::translate("QG_ArcTangentialOptions", "Radius:", 0, QApplication::UnicodeUTF8));
    } // retranslateUi

};

namespace Ui {
    class QG_ArcTangentialOptions: public Ui_QG_ArcTangentialOptions {};
} // namespace Ui

QT_END_NAMESPACE

class QG_ArcTangentialOptions : public QWidget, public Ui::QG_ArcTangentialOptions
{
    Q_OBJECT

public:
    QG_ArcTangentialOptions(QWidget* parent = 0, const char* name = 0, Qt::WindowFlags fl = 0);
    ~QG_ArcTangentialOptions();

public slots:
    virtual void setAction( RS_ActionInterface * a, bool update );
    virtual void updateRadius( const QString & s );

protected:
    RS_ActionDrawArcTangential* action;

protected slots:
    virtual void languageChange();

private:
    void destroy();

};

#endif // QG_ARCTANGENTIALOPTIONS_H
