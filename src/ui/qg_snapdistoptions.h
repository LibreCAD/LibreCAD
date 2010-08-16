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
#ifndef QG_SNAPDISTOPTIONS_H
#define QG_SNAPDISTOPTIONS_H

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
#include "rs_circle.h"
#include "rs_settings.h"

QT_BEGIN_NAMESPACE

class Ui_QG_SnapDistOptions
{
public:
    QHBoxLayout *hboxLayout;
    QLabel *lDist;
    QLineEdit *leDist;
    QFrame *sep1;

    void setupUi(QWidget *QG_SnapDistOptions)
    {
        if (QG_SnapDistOptions->objectName().isEmpty())
            QG_SnapDistOptions->setObjectName(QString::fromUtf8("QG_SnapDistOptions"));
        QG_SnapDistOptions->resize(150, 24);
        QSizePolicy sizePolicy(static_cast<QSizePolicy::Policy>(4), static_cast<QSizePolicy::Policy>(0));
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(QG_SnapDistOptions->sizePolicy().hasHeightForWidth());
        QG_SnapDistOptions->setSizePolicy(sizePolicy);
        QG_SnapDistOptions->setMinimumSize(QSize(130, 22));
        QG_SnapDistOptions->setMaximumSize(QSize(150, 32767));
        hboxLayout = new QHBoxLayout(QG_SnapDistOptions);
        hboxLayout->setSpacing(6);
        hboxLayout->setContentsMargins(1, 1, 1, 1);
        hboxLayout->setObjectName(QString::fromUtf8("hboxLayout"));
        lDist = new QLabel(QG_SnapDistOptions);
        lDist->setObjectName(QString::fromUtf8("lDist"));
        lDist->setWordWrap(false);

        hboxLayout->addWidget(lDist);

        leDist = new QLineEdit(QG_SnapDistOptions);
        leDist->setObjectName(QString::fromUtf8("leDist"));

        hboxLayout->addWidget(leDist);

        sep1 = new QFrame(QG_SnapDistOptions);
        sep1->setObjectName(QString::fromUtf8("sep1"));
        QSizePolicy sizePolicy1(static_cast<QSizePolicy::Policy>(0), static_cast<QSizePolicy::Policy>(1));
        sizePolicy1.setHorizontalStretch(0);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(sep1->sizePolicy().hasHeightForWidth());
        sep1->setSizePolicy(sizePolicy1);
        sep1->setFrameShape(QFrame::VLine);
        sep1->setFrameShadow(QFrame::Sunken);

        hboxLayout->addWidget(sep1);


        retranslateUi(QG_SnapDistOptions);
        QObject::connect(leDist, SIGNAL(textChanged(QString)), QG_SnapDistOptions, SLOT(updateDist(QString)));

        QMetaObject::connectSlotsByName(QG_SnapDistOptions);
    } // setupUi

    void retranslateUi(QWidget *QG_SnapDistOptions)
    {
        QG_SnapDistOptions->setWindowTitle(QApplication::translate("QG_SnapDistOptions", "Snap Distance Options", 0, QApplication::UnicodeUTF8));
        lDist->setText(QApplication::translate("QG_SnapDistOptions", "Distance:", 0, QApplication::UnicodeUTF8));
    } // retranslateUi

};

namespace Ui {
    class QG_SnapDistOptions: public Ui_QG_SnapDistOptions {};
} // namespace Ui

QT_END_NAMESPACE

class QG_SnapDistOptions : public QWidget, public Ui::QG_SnapDistOptions
{
    Q_OBJECT

public:
    QG_SnapDistOptions(QWidget* parent = 0, const char* name = 0, Qt::WindowFlags fl = 0);
    ~QG_SnapDistOptions();

public slots:
    virtual void setDist( double * d );
    virtual void updateDist( const QString & d );

protected:
    double* dist;

protected slots:
    virtual void languageChange();

private:
    void destroy();

};

#endif // QG_SNAPDISTOPTIONS_H
