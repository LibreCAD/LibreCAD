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
#ifndef QG_TEXTOPTIONS_H
#define QG_TEXTOPTIONS_H

#include <qvariant.h>


#include <Qt3Support/Q3MimeSourceFactory>
#include <Qt3Support/Q3TextEdit>
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
#include "rs_actiondrawtext.h"
#include "rs_line.h"
#include "rs_modification.h"
#include "rs_settings.h"

QT_BEGIN_NAMESPACE

class Ui_QG_TextOptions
{
public:
    QHBoxLayout *hboxLayout;
    QLabel *lText;
    Q3TextEdit *teText;
    QLabel *lAngle;
    QLineEdit *leAngle;
    QFrame *sep1;

    void setupUi(QWidget *QG_TextOptions)
    {
        if (QG_TextOptions->objectName().isEmpty())
            QG_TextOptions->setObjectName(QString::fromUtf8("QG_TextOptions"));
        QG_TextOptions->resize(300, 24);
        QSizePolicy sizePolicy(static_cast<QSizePolicy::Policy>(0), static_cast<QSizePolicy::Policy>(0));
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(QG_TextOptions->sizePolicy().hasHeightForWidth());
        QG_TextOptions->setSizePolicy(sizePolicy);
        QG_TextOptions->setMinimumSize(QSize(200, 22));
        QG_TextOptions->setMaximumSize(QSize(300, 32767));
        hboxLayout = new QHBoxLayout(QG_TextOptions);
        hboxLayout->setSpacing(6);
        hboxLayout->setContentsMargins(1, 1, 1, 1);
        hboxLayout->setObjectName(QString::fromUtf8("hboxLayout"));
        lText = new QLabel(QG_TextOptions);
        lText->setObjectName(QString::fromUtf8("lText"));
        lText->setWordWrap(false);

        hboxLayout->addWidget(lText);

        teText = new Q3TextEdit(QG_TextOptions);
        teText->setObjectName(QString::fromUtf8("teText"));
        QSizePolicy sizePolicy1(static_cast<QSizePolicy::Policy>(5), static_cast<QSizePolicy::Policy>(0));
        sizePolicy1.setHorizontalStretch(0);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(teText->sizePolicy().hasHeightForWidth());
        teText->setSizePolicy(sizePolicy1);
        teText->setMinimumSize(QSize(0, 22));
        teText->setMaximumSize(QSize(32767, 22));
        teText->setVScrollBarMode(Q3ScrollView::AlwaysOff);
        teText->setHScrollBarMode(Q3ScrollView::AlwaysOff);

        hboxLayout->addWidget(teText);

        lAngle = new QLabel(QG_TextOptions);
        lAngle->setObjectName(QString::fromUtf8("lAngle"));
        lAngle->setWordWrap(false);

        hboxLayout->addWidget(lAngle);

        leAngle = new QLineEdit(QG_TextOptions);
        leAngle->setObjectName(QString::fromUtf8("leAngle"));

        hboxLayout->addWidget(leAngle);

        sep1 = new QFrame(QG_TextOptions);
        sep1->setObjectName(QString::fromUtf8("sep1"));
        QSizePolicy sizePolicy2(static_cast<QSizePolicy::Policy>(0), static_cast<QSizePolicy::Policy>(1));
        sizePolicy2.setHorizontalStretch(0);
        sizePolicy2.setVerticalStretch(0);
        sizePolicy2.setHeightForWidth(sep1->sizePolicy().hasHeightForWidth());
        sep1->setSizePolicy(sizePolicy2);
        sep1->setFrameShape(QFrame::VLine);
        sep1->setFrameShadow(QFrame::Sunken);

        hboxLayout->addWidget(sep1);


        retranslateUi(QG_TextOptions);
        QObject::connect(teText, SIGNAL(textChanged()), QG_TextOptions, SLOT(updateText()));
        QObject::connect(leAngle, SIGNAL(textChanged(QString)), QG_TextOptions, SLOT(updateAngle()));

        QMetaObject::connectSlotsByName(QG_TextOptions);
    } // setupUi

    void retranslateUi(QWidget *QG_TextOptions)
    {
        QG_TextOptions->setWindowTitle(QApplication::translate("QG_TextOptions", "Text Options", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        QG_TextOptions->setProperty("toolTip", QVariant(QString()));
#endif // QT_NO_TOOLTIP
        lText->setText(QApplication::translate("QG_TextOptions", "Text:", 0, QApplication::UnicodeUTF8));
        lAngle->setText(QApplication::translate("QG_TextOptions", "Angle:", 0, QApplication::UnicodeUTF8));
    } // retranslateUi

};

namespace Ui {
    class QG_TextOptions: public Ui_QG_TextOptions {};
} // namespace Ui

QT_END_NAMESPACE

class QG_TextOptions : public QWidget, public Ui::QG_TextOptions
{
    Q_OBJECT

public:
    QG_TextOptions(QWidget* parent = 0, const char* name = 0, Qt::WindowFlags fl = 0);
    ~QG_TextOptions();

public slots:
    virtual void setAction( RS_ActionInterface * a, bool update );
    virtual void updateText();
    virtual void updateAngle();

protected:
    RS_ActionDrawText* action;

protected slots:
    virtual void languageChange();

};

#endif // QG_TEXTOPTIONS_H
