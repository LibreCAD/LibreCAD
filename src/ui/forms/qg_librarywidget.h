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
#ifndef QG_LIBRARYWIDGET_H
#define QG_LIBRARYWIDGET_H

#include <qvariant.h>


#include <Qt3Support/Q3Header>
#include <Qt3Support/Q3IconView>
#include <Qt3Support/Q3ListView>
#include <Qt3Support/Q3MimeSourceFactory>
#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QHeaderView>
#include <QtGui/QPushButton>
#include <QtGui/QVBoxLayout>
#include <QtGui/QWidget>
#include <QtCore/QDir>
#include <QtCore/QMutableMapIterator>
#include "qg_actionhandler.h"
#include "qg_listviewitem.h"

QT_BEGIN_NAMESPACE

class Ui_QG_LibraryWidget
{
public:
    QVBoxLayout *vboxLayout;
    Q3ListView *lvDirectory;
    Q3IconView *ivPreview;
    QPushButton *bInsert;

    void setupUi(QWidget *QG_LibraryWidget)
    {
        if (QG_LibraryWidget->objectName().isEmpty())
            QG_LibraryWidget->setObjectName(QString::fromUtf8("QG_LibraryWidget"));
        QG_LibraryWidget->resize(146, 413);
        vboxLayout = new QVBoxLayout(QG_LibraryWidget);
        vboxLayout->setSpacing(2);
        vboxLayout->setContentsMargins(2, 2, 2, 2);
        vboxLayout->setObjectName(QString::fromUtf8("vboxLayout"));
        lvDirectory = new Q3ListView(QG_LibraryWidget);
        lvDirectory->addColumn(QApplication::translate("QG_LibraryWidget", "Directories", 0, QApplication::UnicodeUTF8));
        lvDirectory->header()->setClickEnabled(false, lvDirectory->header()->count() - 1);
        lvDirectory->header()->setResizeEnabled(false, lvDirectory->header()->count() - 1);
        lvDirectory->setObjectName(QString::fromUtf8("lvDirectory"));
        QSizePolicy sizePolicy(static_cast<QSizePolicy::Policy>(7), static_cast<QSizePolicy::Policy>(13));
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(lvDirectory->sizePolicy().hasHeightForWidth());
        lvDirectory->setSizePolicy(sizePolicy);
        lvDirectory->setMinimumSize(QSize(0, 32));
        lvDirectory->setRootIsDecorated(true);
        lvDirectory->setResizeMode(Q3ListView::LastColumn);
        lvDirectory->setTreeStepSize(14);

        vboxLayout->addWidget(lvDirectory);

        ivPreview = new Q3IconView(QG_LibraryWidget);
        ivPreview->setObjectName(QString::fromUtf8("ivPreview"));
        sizePolicy.setHeightForWidth(ivPreview->sizePolicy().hasHeightForWidth());
        ivPreview->setSizePolicy(sizePolicy);
        ivPreview->setMinimumSize(QSize(0, 32));
        ivPreview->setAcceptDrops(false);
        ivPreview->setResizePolicy(Q3ScrollView::AutoOneFit);
        ivPreview->setDragAutoScroll(false);
        ivPreview->setSpacing(2);
        ivPreview->setArrangement(Q3IconView::LeftToRight);
        ivPreview->setResizeMode(Q3IconView::Adjust);
        ivPreview->setMaxItemWidth(64);
        ivPreview->setAutoArrange(true);
        ivPreview->setItemsMovable(false);
        ivPreview->setWordWrapIconText(false);
        ivPreview->setShowToolTips(true);

        vboxLayout->addWidget(ivPreview);

        bInsert = new QPushButton(QG_LibraryWidget);
        bInsert->setObjectName(QString::fromUtf8("bInsert"));

        vboxLayout->addWidget(bInsert);


        retranslateUi(QG_LibraryWidget);
        QObject::connect(lvDirectory, SIGNAL(currentChanged(Q3ListViewItem*)), QG_LibraryWidget, SLOT(updatePreview(Q3ListViewItem*)));
        QObject::connect(bInsert, SIGNAL(clicked()), QG_LibraryWidget, SLOT(insert()));

        QMetaObject::connectSlotsByName(QG_LibraryWidget);
    } // setupUi

    void retranslateUi(QWidget *QG_LibraryWidget)
    {
        QG_LibraryWidget->setWindowTitle(QApplication::translate("QG_LibraryWidget", "Library Browser", 0, QApplication::UnicodeUTF8));
        lvDirectory->header()->setLabel(0, QApplication::translate("QG_LibraryWidget", "Directories", 0, QApplication::UnicodeUTF8));
        bInsert->setText(QApplication::translate("QG_LibraryWidget", "Insert", 0, QApplication::UnicodeUTF8));
    } // retranslateUi

};

namespace Ui {
    class QG_LibraryWidget: public Ui_QG_LibraryWidget {};
} // namespace Ui

QT_END_NAMESPACE

class QG_LibraryWidget : public QWidget, public Ui::QG_LibraryWidget
{
    Q_OBJECT

public:
    QG_LibraryWidget(QWidget* parent = 0, const char* name = 0, Qt::WindowFlags fl = 0);
    ~QG_LibraryWidget();

    virtual QString getItemDir( Q3ListViewItem * item );
    virtual QString getItemPath( Q3IconViewItem * item );
    virtual QPixmap getPixmap( const QString & dir, const QString & dxfFile, const QString & dxfPath );
    virtual QString getPathToPixmap( const QString & dir, const QString & dxfFile, const QString & dxfPath );

public slots:
    virtual void setActionHandler( QG_ActionHandler * ah );
    virtual void keyPressEvent( QKeyEvent * e );
    virtual void insert();
    virtual void appendTree( QG_ListViewItem * item, QString directory );
    virtual void updatePreview( Q3ListViewItem * item );

signals:
    void escape();

protected slots:
    virtual void languageChange();

private:
    QG_ActionHandler* actionHandler;

    void init();

};

#endif // QG_LIBRARYWIDGET_H
