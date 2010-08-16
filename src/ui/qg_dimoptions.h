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
#ifndef QG_DIMOPTIONS_H
#define QG_DIMOPTIONS_H

#include <qvariant.h>


#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QComboBox>
#include <QtGui/QFrame>
#include <QtGui/QHBoxLayout>
#include <QtGui/QHeaderView>
#include <QtGui/QLabel>
#include <QtGui/QLineEdit>
#include <QtGui/QToolButton>
#include <QtGui/QWidget>
#include "rs_actiondimension.h"
#include "rs_dimension.h"
#include "rs_settings.h"

QT_BEGIN_NAMESPACE

class Ui_QG_DimOptions
{
public:
    QHBoxLayout *hboxLayout;
    QLabel *lLabel;
    QToolButton *bDiameter;
    QLineEdit *leLabel;
    QComboBox *cbSymbol;
    QLabel *lTol1;
    QLineEdit *leTol1;
    QLabel *lTol2;
    QLineEdit *leTol2;
    QFrame *sep1;

    void setupUi(QWidget *QG_DimOptions)
    {
        if (QG_DimOptions->objectName().isEmpty())
            QG_DimOptions->setObjectName(QString::fromUtf8("QG_DimOptions"));
        QG_DimOptions->resize(420, 22);
        QSizePolicy sizePolicy(static_cast<QSizePolicy::Policy>(4), static_cast<QSizePolicy::Policy>(5));
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(QG_DimOptions->sizePolicy().hasHeightForWidth());
        QG_DimOptions->setSizePolicy(sizePolicy);
        QG_DimOptions->setMinimumSize(QSize(420, 22));
        QG_DimOptions->setMaximumSize(QSize(420, 22));
        hboxLayout = new QHBoxLayout(QG_DimOptions);
        hboxLayout->setSpacing(6);
        hboxLayout->setContentsMargins(1, 1, 1, 1);
        hboxLayout->setObjectName(QString::fromUtf8("hboxLayout"));
        lLabel = new QLabel(QG_DimOptions);
        lLabel->setObjectName(QString::fromUtf8("lLabel"));
        QSizePolicy sizePolicy1(static_cast<QSizePolicy::Policy>(0), static_cast<QSizePolicy::Policy>(0));
        sizePolicy1.setHorizontalStretch(0);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(lLabel->sizePolicy().hasHeightForWidth());
        lLabel->setSizePolicy(sizePolicy1);
        lLabel->setFrameShape(QFrame::NoFrame);
        lLabel->setFrameShadow(QFrame::Plain);
        lLabel->setWordWrap(false);

        hboxLayout->addWidget(lLabel);

        bDiameter = new QToolButton(QG_DimOptions);
        bDiameter->setObjectName(QString::fromUtf8("bDiameter"));
        QSizePolicy sizePolicy2(static_cast<QSizePolicy::Policy>(1), static_cast<QSizePolicy::Policy>(0));
        sizePolicy2.setHorizontalStretch(0);
        sizePolicy2.setVerticalStretch(0);
        sizePolicy2.setHeightForWidth(bDiameter->sizePolicy().hasHeightForWidth());
        bDiameter->setSizePolicy(sizePolicy2);
        bDiameter->setIcon(qt_get_icon(image0_ID));
        bDiameter->setCheckable(true);

        hboxLayout->addWidget(bDiameter);

        leLabel = new QLineEdit(QG_DimOptions);
        leLabel->setObjectName(QString::fromUtf8("leLabel"));
        QSizePolicy sizePolicy3(static_cast<QSizePolicy::Policy>(5), static_cast<QSizePolicy::Policy>(0));
        sizePolicy3.setHorizontalStretch(0);
        sizePolicy3.setVerticalStretch(0);
        sizePolicy3.setHeightForWidth(leLabel->sizePolicy().hasHeightForWidth());
        leLabel->setSizePolicy(sizePolicy3);

        hboxLayout->addWidget(leLabel);

        cbSymbol = new QComboBox(QG_DimOptions);
        cbSymbol->setObjectName(QString::fromUtf8("cbSymbol"));
        QSizePolicy sizePolicy4(static_cast<QSizePolicy::Policy>(3), static_cast<QSizePolicy::Policy>(0));
        sizePolicy4.setHorizontalStretch(0);
        sizePolicy4.setVerticalStretch(0);
        sizePolicy4.setHeightForWidth(cbSymbol->sizePolicy().hasHeightForWidth());
        cbSymbol->setSizePolicy(sizePolicy4);

        hboxLayout->addWidget(cbSymbol);

        lTol1 = new QLabel(QG_DimOptions);
        lTol1->setObjectName(QString::fromUtf8("lTol1"));
        lTol1->setPixmap(qt_get_icon(image1_ID));
        lTol1->setWordWrap(false);

        hboxLayout->addWidget(lTol1);

        leTol1 = new QLineEdit(QG_DimOptions);
        leTol1->setObjectName(QString::fromUtf8("leTol1"));
        sizePolicy3.setHeightForWidth(leTol1->sizePolicy().hasHeightForWidth());
        leTol1->setSizePolicy(sizePolicy3);

        hboxLayout->addWidget(leTol1);

        lTol2 = new QLabel(QG_DimOptions);
        lTol2->setObjectName(QString::fromUtf8("lTol2"));
        lTol2->setPixmap(qt_get_icon(image2_ID));
        lTol2->setWordWrap(false);

        hboxLayout->addWidget(lTol2);

        leTol2 = new QLineEdit(QG_DimOptions);
        leTol2->setObjectName(QString::fromUtf8("leTol2"));
        sizePolicy3.setHeightForWidth(leTol2->sizePolicy().hasHeightForWidth());
        leTol2->setSizePolicy(sizePolicy3);

        hboxLayout->addWidget(leTol2);

        sep1 = new QFrame(QG_DimOptions);
        sep1->setObjectName(QString::fromUtf8("sep1"));
        QSizePolicy sizePolicy5(static_cast<QSizePolicy::Policy>(0), static_cast<QSizePolicy::Policy>(1));
        sizePolicy5.setHorizontalStretch(0);
        sizePolicy5.setVerticalStretch(0);
        sizePolicy5.setHeightForWidth(sep1->sizePolicy().hasHeightForWidth());
        sep1->setSizePolicy(sizePolicy5);
        sep1->setFrameShape(QFrame::VLine);
        sep1->setFrameShadow(QFrame::Sunken);

        hboxLayout->addWidget(sep1);


        retranslateUi(QG_DimOptions);
        QObject::connect(leLabel, SIGNAL(textChanged(QString)), QG_DimOptions, SLOT(updateLabel()));
        QObject::connect(bDiameter, SIGNAL(toggled(bool)), QG_DimOptions, SLOT(updateLabel()));
        QObject::connect(leTol1, SIGNAL(textChanged(QString)), QG_DimOptions, SLOT(updateLabel()));
        QObject::connect(leTol2, SIGNAL(textChanged(QString)), QG_DimOptions, SLOT(updateLabel()));
        QObject::connect(cbSymbol, SIGNAL(activated(QString)), QG_DimOptions, SLOT(insertSign(QString)));

        QMetaObject::connectSlotsByName(QG_DimOptions);
    } // setupUi

    void retranslateUi(QWidget *QG_DimOptions)
    {
        QG_DimOptions->setWindowTitle(QApplication::translate("QG_DimOptions", "Dimension Options", 0, QApplication::UnicodeUTF8));
        lLabel->setText(QApplication::translate("QG_DimOptions", "Label:", 0, QApplication::UnicodeUTF8));
        bDiameter->setText(QString());
        cbSymbol->clear();
        cbSymbol->insertItems(0, QStringList()
         << QApplication::translate("QG_DimOptions", "\303\270", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("QG_DimOptions", "\302\260", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("QG_DimOptions", "\302\261", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("QG_DimOptions", "\302\266", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("QG_DimOptions", "\303\227", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("QG_DimOptions", "\303\267", 0, QApplication::UnicodeUTF8)
        );
        lTol1->setText(QString());
        lTol2->setText(QString());
    } // retranslateUi


protected:
    enum IconID
    {
        image0_ID,
        image1_ID,
        image2_ID,
        unknown_ID
    };
    static QPixmap qt_get_icon(IconID id)
    {
    static const char* const image0_data[] = { 
"15 15 2 1",
". c None",
"# c #000000",
"...............",
"...............",
"...............",
".....####.#....",
"....#....#.....",
"...#....#.#....",
"...#...#..#....",
"...#..#...#....",
"...#.#....#....",
"....#....#.....",
"...#.####......",
"...............",
"...............",
"...............",
"..............."};


    static const char* const image1_data[] = { 
"32 10 3 1",
"# c None",
". c #000000",
"a c #ff0000",
"..................##............",
".################.##.aaaaaaaaaa.",
".################.##.aaaaaaaaaa.",
".################.##............",
".################.##############",
".################.##############",
".################.##............",
".################.##.##########.",
".################.##.##########.",
"..................##............"};


    static const char* const image2_data[] = { 
"32 10 3 1",
"# c None",
". c #000000",
"a c #ff0000",
"..................##............",
".################.##.##########.",
".################.##.##########.",
".################.##............",
".################.##############",
".################.##############",
".################.##............",
".################.##.aaaaaaaaaa.",
".################.##.aaaaaaaaaa.",
"..................##............"};


    switch (id) {
        case image0_ID: return QPixmap((const char**)image0_data);
        case image1_ID: return QPixmap((const char**)image1_data);
        case image2_ID: return QPixmap((const char**)image2_data);
        default: return QPixmap();
    } // switch
    } // icon

};

namespace Ui {
    class QG_DimOptions: public Ui_QG_DimOptions {};
} // namespace Ui

QT_END_NAMESPACE

class QG_DimOptions : public QWidget, public Ui::QG_DimOptions
{
    Q_OBJECT

public:
    QG_DimOptions(QWidget* parent = 0, const char* name = 0, Qt::WindowFlags fl = 0);
    ~QG_DimOptions();

public slots:
    virtual void setAction( RS_ActionInterface * a, bool update );
    virtual void updateLabel();
    virtual void insertSign( const QString & c );

protected:
    RS_ActionDimension* action;

protected slots:
    virtual void languageChange();

private:
    void destroy();

};

#endif // QG_DIMOPTIONS_H
