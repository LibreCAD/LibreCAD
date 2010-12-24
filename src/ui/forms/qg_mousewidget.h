/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2010 R. van Twisk (librecad@rvt.dds.nl)
** Copyright (C) 2001-2003 RibbonSoft. All rights reserved.
**
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by 
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
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
#ifndef QG_MOUSEWIDGET_H
#define QG_MOUSEWIDGET_H

#include <qvariant.h>


#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QGridLayout>
#include <QtGui/QHeaderView>
#include <QtGui/QLabel>
#include <QtGui/QWidget>

QT_BEGIN_NAMESPACE

class Ui_QG_MouseWidget
{
public:
    QGridLayout *gridLayout;
    QLabel *lRightButton;
    QLabel *lLeftButton;
    QLabel *lMouse;

    void setupUi(QWidget *QG_MouseWidget)
    {
        if (QG_MouseWidget->objectName().isEmpty())
            QG_MouseWidget->setObjectName(QString::fromUtf8("QG_MouseWidget"));
        QG_MouseWidget->resize(318, 28);
        QSizePolicy sizePolicy(static_cast<QSizePolicy::Policy>(5), static_cast<QSizePolicy::Policy>(0));
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(QG_MouseWidget->sizePolicy().hasHeightForWidth());
        QG_MouseWidget->setSizePolicy(sizePolicy);
        QG_MouseWidget->setMinimumSize(QSize(300, 28));
        QG_MouseWidget->setMaximumSize(QSize(500, 50));
        gridLayout = new QGridLayout(QG_MouseWidget);
        gridLayout->setSpacing(5);
        gridLayout->setContentsMargins(0, 0, 0, 0);
        gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
        lRightButton = new QLabel(QG_MouseWidget);
        lRightButton->setObjectName(QString::fromUtf8("lRightButton"));
        sizePolicy.setHeightForWidth(lRightButton->sizePolicy().hasHeightForWidth());
        lRightButton->setSizePolicy(sizePolicy);
        lRightButton->setMinimumSize(QSize(0, 28));
        lRightButton->setMaximumSize(QSize(32767, 28));
        QFont font;
        font.setFamily(QString::fromUtf8("Helvetica"));
        lRightButton->setFont(font);
        lRightButton->setAlignment(Qt::AlignTop|Qt::AlignLeft);
        lRightButton->setWordWrap(false);

        gridLayout->addWidget(lRightButton, 0, 2, 1, 1);

        lLeftButton = new QLabel(QG_MouseWidget);
        lLeftButton->setObjectName(QString::fromUtf8("lLeftButton"));
        sizePolicy.setHeightForWidth(lLeftButton->sizePolicy().hasHeightForWidth());
        lLeftButton->setSizePolicy(sizePolicy);
        lLeftButton->setMinimumSize(QSize(0, 27));
        lLeftButton->setMaximumSize(QSize(32767, 28));
        lLeftButton->setFont(font);
        lLeftButton->setFrameShape(QFrame::NoFrame);
        lLeftButton->setFrameShadow(QFrame::Plain);
        lLeftButton->setAlignment(Qt::AlignTop|Qt::AlignRight);
        lLeftButton->setWordWrap(false);

        gridLayout->addWidget(lLeftButton, 0, 0, 1, 1);

        lMouse = new QLabel(QG_MouseWidget);
        lMouse->setObjectName(QString::fromUtf8("lMouse"));
        QSizePolicy sizePolicy1(static_cast<QSizePolicy::Policy>(0), static_cast<QSizePolicy::Policy>(0));
        sizePolicy1.setHorizontalStretch(0);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(lMouse->sizePolicy().hasHeightForWidth());
        lMouse->setSizePolicy(sizePolicy1);
        lMouse->setMinimumSize(QSize(16, 27));
        lMouse->setMaximumSize(QSize(16, 27));
        lMouse->setPixmap(qt_get_icon(image0_ID));
        lMouse->setAlignment(Qt::AlignCenter);
        lMouse->setWordWrap(false);

        gridLayout->addWidget(lMouse, 0, 1, 1, 1);


        retranslateUi(QG_MouseWidget);

        QMetaObject::connectSlotsByName(QG_MouseWidget);
    } // setupUi

    void retranslateUi(QWidget *QG_MouseWidget)
    {
        QG_MouseWidget->setWindowTitle(QApplication::translate("QG_MouseWidget", "Mouse", 0, QApplication::UnicodeUTF8));
        lRightButton->setText(QApplication::translate("QG_MouseWidget", "Right", 0, QApplication::UnicodeUTF8));
        lLeftButton->setText(QApplication::translate("QG_MouseWidget", "Left", 0, QApplication::UnicodeUTF8));
    } // retranslateUi


protected:
    enum IconID
    {
        image0_ID,
        unknown_ID
    };
    static QPixmap qt_get_icon(IconID id)
    {
    static const char* const image0_data[] = { 
"16 27 4 1",
". c None",
"# c #000000",
"a c #bdbebd",
"b c #ffffff",
"......#a#.......",
"......#a#.......",
".......#a#......",
".......#a#......",
".......#a#......",
".##############.",
"#aabb#aabb#aabb#",
"#abbb#abbb#abbb#",
"#abbb#abbb#abbb#",
"#abbb#abbb#abbb#",
"#abbb#abbb#abbb#",
"#abbb#abbb#abbb#",
"################",
"#bbbbbbbbbbbbbb#",
"#babbbbbbbbbbbb#",
"#bbbbbbbbbbbbbb#",
"#babbbbbbbbbbbb#",
"#abbabbbbbbbbbb#",
"#aabbbbbbbbbbbb#",
"#ababbbbbbbbbbb#",
"#aaabbbbbbbbbbb#",
"#aabababbbbbbbb#",
"#aaabbbbbbbbbbb#",
".#aaaabbabbbbb#.",
"..#aababbbbbb##.",
"...##ababbb##...",
".....######....."};


    switch (id) {
        case image0_ID: return QPixmap((const char**)image0_data);
        default: return QPixmap();
    } // switch
    } // icon

};

namespace Ui {
    class QG_MouseWidget: public Ui_QG_MouseWidget {};
} // namespace Ui

QT_END_NAMESPACE

class QG_MouseWidget : public QWidget, public Ui::QG_MouseWidget
{
    Q_OBJECT

public:
    QG_MouseWidget(QWidget* parent = 0, const char* name = 0, Qt::WindowFlags fl = 0);
    ~QG_MouseWidget();

public slots:
    virtual void init();
    virtual void setHelp( const QString & left, const QString & right );

protected slots:
    virtual void languageChange();

};

#endif // QG_MOUSEWIDGET_H
