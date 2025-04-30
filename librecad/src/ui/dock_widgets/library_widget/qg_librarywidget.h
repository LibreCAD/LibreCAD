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
#ifndef QG_LIBRARYWIDGET_H
#define QG_LIBRARYWIDGET_H

#include <memory>
#include "lc_graphicviewawarewidget.h"
#include <QModelIndex>

class QG_ActionHandler;
class QListView;
class QPushButton;
class QStandardItemModel;
class QStandardItem;
class QTreeView;

class QG_LibraryWidget : public LC_GraphicViewAwareWidget{
    Q_OBJECT
public:
    QG_LibraryWidget(QG_ActionHandler *action_handler, QWidget* parent = nullptr, const char* name = nullptr, Qt::WindowFlags fl = {});
    ~QG_LibraryWidget() override;

    QPushButton* getInsertButton() const{
        return bInsert;
    }

    void setGraphicView(RS_GraphicView* gview) override;
private:
    QPushButton *bInsert=nullptr;
    QString getItemDir( QStandardItem * item );
    QString getItemPath( QStandardItem * item );
    QIcon getIcon( const QString & dir, const QString & dxfFile, const QString & dxfPath );
    QString getPathToPixmap( const QString & dir, const QString & dxfFile, const QString & dxfPath );
public slots:
    void setActionHandler( QG_ActionHandler * ah );
    void keyPressEvent( QKeyEvent *e ) override;
    void insert();
    void refresh();
    void scanTree();
    void buildTree();
    void appendTree( QStandardItem * item, QString directory );
    void updatePreview( QModelIndex idx );
    void expandView( QModelIndex idx );
    void collapseView( QModelIndex idx );
    void updateWidgetSettings();
signals:
    void escape();private:
    QG_ActionHandler* actionHandler = nullptr;
    std::unique_ptr<QStandardItemModel> dirModel;
    std::unique_ptr<QStandardItemModel> iconModel;
    QTreeView *dirView = nullptr;
    QListView *ivPreview = nullptr;
    QPushButton *bRefresh = nullptr;
    QPushButton *bRebuild = nullptr;
};

#endif // QG_LIBRARYWIDGET_H
