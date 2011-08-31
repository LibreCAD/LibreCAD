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

#include <QWidget>
#include <QModelIndex>

class QG_ActionHandler;
class QStandardItemModel;
class QStandardItem;
class QTreeView;
class QListView;
class QPushButton;

class QG_LibraryWidget : public QWidget
{
    Q_OBJECT

public:
    QG_LibraryWidget(QWidget* parent = 0, const char* name = 0, Qt::WindowFlags fl = 0);
    ~QG_LibraryWidget();

    QPushButton *bInsert; //RLZ change bInsert to private
private:
    virtual QString getItemDir( QStandardItem * item );
    virtual QString getItemPath( QStandardItem * item );
    virtual QIcon getIcon( const QString & dir, const QString & dxfFile, const QString & dxfPath );
    virtual QString getPathToPixmap( const QString & dir, const QString & dxfFile, const QString & dxfPath );

public slots:
    virtual void setActionHandler( QG_ActionHandler * ah );
    virtual void keyPressEvent( QKeyEvent * e );
    virtual void insert();
    virtual void appendTree( QStandardItem * item, QString directory );
    virtual void updatePreview( QModelIndex idx );
    virtual void expandView( QModelIndex idx );
    virtual void collapseView( QModelIndex idx );

signals:
    void escape();

protected slots:
    virtual void languageChange();

private:
    QG_ActionHandler* actionHandler;
    QStandardItemModel *dirModel;
    QStandardItemModel *iconModel;
    QTreeView *dirView;
    QListView *ivPreview;
};

#endif // QG_LIBRARYWIDGET_H
