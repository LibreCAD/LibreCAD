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
#ifndef QG_LINEPOLYGON2OPTIONS_H
#define QG_LINEPOLYGON2OPTIONS_H

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
#include <QtGui/QSpinBox>
#include <QtGui/QWidget>
#include "rs_actiondrawlinepolygon2.h"
#include "rs_line.h"
#include "rs_settings.h"

QT_BEGIN_NAMESPACE

class Ui_QG_LinePolygon2Options
{
public:
    QHBoxLayout *hboxLayout;
    QLabel *lNumber;
    QSpinBox *sbNumber;
    QFrame *sep1;

    void setupUi(QWidget *QG_LinePolygon2Options)
    {
        if (QG_LinePolygon2Options->objectName().isEmpty())
            QG_LinePolygon2Options->setObjectName(QString::fromUtf8("QG_LinePolygon2Options"));
        QG_LinePolygon2Options->resize(140, 22);
        QSizePolicy sizePolicy(static_cast<QSizePolicy::Policy>(4), static_cast<QSizePolicy::Policy>(5));
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(QG_LinePolygon2Options->sizePolicy().hasHeightForWidth());
        QG_LinePolygon2Options->setSizePolicy(sizePolicy);
        QG_LinePolygon2Options->setMinimumSize(QSize(100, 22));
        QG_LinePolygon2Options->setMaximumSize(QSize(140, 22));
        hboxLayout = new QHBoxLayout(QG_LinePolygon2Options);
        hboxLayout->setSpacing(6);
        hboxLayout->setContentsMargins(1, 1, 1, 1);
        hboxLayout->setObjectName(QString::fromUtf8("hboxLayout"));
        lNumber = new QLabel(QG_LinePolygon2Options);
        lNumber->setObjectName(QString::fromUtf8("lNumber"));
        QSizePolicy sizePolicy1(static_cast<QSizePolicy::Policy>(1), static_cast<QSizePolicy::Policy>(0));
        sizePolicy1.setHorizontalStretch(0);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(lNumber->sizePolicy().hasHeightForWidth());
        lNumber->setSizePolicy(sizePolicy1);
        lNumber->setWordWrap(false);

        hboxLayout->addWidget(lNumber);

        sbNumber = new QSpinBox(QG_LinePolygon2Options);
        sbNumber->setObjectName(QString::fromUtf8("sbNumber"));
        sbNumber->setMaximum(9999);
        sbNumber->setMinimum(3);

        hboxLayout->addWidget(sbNumber);

        sep1 = new QFrame(QG_LinePolygon2Options);
        sep1->setObjectName(QString::fromUtf8("sep1"));
        QSizePolicy sizePolicy2(static_cast<QSizePolicy::Policy>(0), static_cast<QSizePolicy::Policy>(1));
        sizePolicy2.setHorizontalStretch(0);
        sizePolicy2.setVerticalStretch(0);
        sizePolicy2.setHeightForWidth(sep1->sizePolicy().hasHeightForWidth());
        sep1->setSizePolicy(sizePolicy2);
        sep1->setFrameShape(QFrame::VLine);
        sep1->setFrameShadow(QFrame::Sunken);

        hboxLayout->addWidget(sep1);


        retranslateUi(QG_LinePolygon2Options);
        QObject::connect(sbNumber, SIGNAL(valueChanged(int)), QG_LinePolygon2Options, SLOT(updateNumber(int)));

        QMetaObject::connectSlotsByName(QG_LinePolygon2Options);
    } // setupUi

    void retranslateUi(QWidget *QG_LinePolygon2Options)
    {
        QG_LinePolygon2Options->setWindowTitle(QApplication::translate("QG_LinePolygon2Options", "Polygon Options", 0, QApplication::UnicodeUTF8));
        lNumber->setText(QApplication::translate("QG_LinePolygon2Options", "Number:", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        sbNumber->setProperty("toolTip", QVariant(QApplication::translate("QG_LinePolygon2Options", "Number of edges", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
    } // retranslateUi

};

namespace Ui {
    class QG_LinePolygon2Options: public Ui_QG_LinePolygon2Options {};
} // namespace Ui

QT_END_NAMESPACE

class QG_LinePolygon2Options : public QWidget, public Ui::QG_LinePolygon2Options
{
    Q_OBJECT

public:
    QG_LinePolygon2Options(QWidget* parent = 0, const char* name = 0, Qt::WindowFlags fl = 0);
    ~QG_LinePolygon2Options();

public slots:
    virtual void setAction( RS_ActionInterface * a, bool update );
    virtual void updateNumber( int n );

protected:
    RS_ActionDrawLinePolygon2* action;

protected slots:
    virtual void languageChange();

private:
    void destroy();

};

#endif // QG_LINEPOLYGON2OPTIONS_H
