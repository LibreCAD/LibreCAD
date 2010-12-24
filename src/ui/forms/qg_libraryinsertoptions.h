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
#ifndef QG_LIBRARYINSERTOPTIONS_H
#define QG_LIBRARYINSERTOPTIONS_H

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
#include "rs_actionlibraryinsert.h"
#include "rs_line.h"
#include "rs_modification.h"
#include "rs_settings.h"

QT_BEGIN_NAMESPACE

class Ui_QG_LibraryInsertOptions
{
public:
    QHBoxLayout *hboxLayout;
    QLabel *lAngle;
    QLineEdit *leAngle;
    QLabel *lFactor;
    QLineEdit *leFactor;
    QFrame *sep1;

    void setupUi(QWidget *QG_LibraryInsertOptions)
    {
        if (QG_LibraryInsertOptions->objectName().isEmpty())
            QG_LibraryInsertOptions->setObjectName(QString::fromUtf8("QG_LibraryInsertOptions"));
        QG_LibraryInsertOptions->resize(240, 24);
        QSizePolicy sizePolicy(static_cast<QSizePolicy::Policy>(4), static_cast<QSizePolicy::Policy>(0));
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(QG_LibraryInsertOptions->sizePolicy().hasHeightForWidth());
        QG_LibraryInsertOptions->setSizePolicy(sizePolicy);
        QG_LibraryInsertOptions->setMinimumSize(QSize(240, 22));
        QG_LibraryInsertOptions->setMaximumSize(QSize(300, 32767));
        hboxLayout = new QHBoxLayout(QG_LibraryInsertOptions);
        hboxLayout->setSpacing(6);
        hboxLayout->setContentsMargins(1, 1, 1, 1);
        hboxLayout->setObjectName(QString::fromUtf8("hboxLayout"));
        lAngle = new QLabel(QG_LibraryInsertOptions);
        lAngle->setObjectName(QString::fromUtf8("lAngle"));
        lAngle->setWordWrap(false);

        hboxLayout->addWidget(lAngle);

        leAngle = new QLineEdit(QG_LibraryInsertOptions);
        leAngle->setObjectName(QString::fromUtf8("leAngle"));

        hboxLayout->addWidget(leAngle);

        lFactor = new QLabel(QG_LibraryInsertOptions);
        lFactor->setObjectName(QString::fromUtf8("lFactor"));
        lFactor->setWordWrap(false);

        hboxLayout->addWidget(lFactor);

        leFactor = new QLineEdit(QG_LibraryInsertOptions);
        leFactor->setObjectName(QString::fromUtf8("leFactor"));

        hboxLayout->addWidget(leFactor);

        sep1 = new QFrame(QG_LibraryInsertOptions);
        sep1->setObjectName(QString::fromUtf8("sep1"));
        QSizePolicy sizePolicy1(static_cast<QSizePolicy::Policy>(0), static_cast<QSizePolicy::Policy>(1));
        sizePolicy1.setHorizontalStretch(0);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(sep1->sizePolicy().hasHeightForWidth());
        sep1->setSizePolicy(sizePolicy1);
        sep1->setFrameShape(QFrame::VLine);
        sep1->setFrameShadow(QFrame::Sunken);

        hboxLayout->addWidget(sep1);


        retranslateUi(QG_LibraryInsertOptions);
        QObject::connect(leAngle, SIGNAL(textChanged(QString)), QG_LibraryInsertOptions, SLOT(updateData()));
        QObject::connect(leFactor, SIGNAL(textChanged(QString)), QG_LibraryInsertOptions, SLOT(updateData()));

        QMetaObject::connectSlotsByName(QG_LibraryInsertOptions);
    } // setupUi

    void retranslateUi(QWidget *QG_LibraryInsertOptions)
    {
        QG_LibraryInsertOptions->setWindowTitle(QApplication::translate("QG_LibraryInsertOptions", "Library Insert Options", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        QG_LibraryInsertOptions->setProperty("toolTip", QVariant(QString()));
#endif // QT_NO_TOOLTIP
        lAngle->setText(QApplication::translate("QG_LibraryInsertOptions", "Angle:", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        leAngle->setProperty("toolTip", QVariant(QApplication::translate("QG_LibraryInsertOptions", "Rotation Angle", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
        lFactor->setText(QApplication::translate("QG_LibraryInsertOptions", "Factor:", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        leFactor->setProperty("toolTip", QVariant(QApplication::translate("QG_LibraryInsertOptions", "Scale Factor", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
    } // retranslateUi

};

namespace Ui {
    class QG_LibraryInsertOptions: public Ui_QG_LibraryInsertOptions {};
} // namespace Ui

QT_END_NAMESPACE

class QG_LibraryInsertOptions : public QWidget, public Ui::QG_LibraryInsertOptions
{
    Q_OBJECT

public:
    QG_LibraryInsertOptions(QWidget* parent = 0, const char* name = 0, Qt::WindowFlags fl = 0);
    ~QG_LibraryInsertOptions();

public slots:
    virtual void setAction( RS_ActionInterface * a, bool update );
    virtual void updateData();

protected:
    RS_ActionLibraryInsert* action;

protected slots:
    virtual void languageChange();

private:
    void destroy();

};

#endif // QG_LIBRARYINSERTOPTIONS_H
