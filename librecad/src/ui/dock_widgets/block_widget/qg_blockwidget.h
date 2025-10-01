/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2011 Rallaz (rallazz@gmail.com)
** Copyright (C) 2010 R. van Twisk (librecad@rvt.dds.nl)
**
**
** This file is free software; you can redistribute it and/or modify
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

#ifndef QG_BLOCKWIDGET_H
#define QG_BLOCKWIDGET_H

#include <QAbstractTableModel>
#include <QIcon>
#include <QSortFilterProxyModel>

#include "lc_graphicviewawarewidget.h"
#include "rs_blocklistlistener.h"

class LC_ActionGroupManager;
class QG_ActionHandler;
class QItemSelection;
class QTableView;
class QLineEdit;

class RS_Block;
class RS_BlockList;
class QG_BlockModel;
class LC_FlexLayout;

/**
 * Implementation of a model to use in QG_BlockWidget
 */
class QG_BlockModel: public QAbstractTableModel {
public:
    enum {
        VISIBLE,
        NAME,
        LAST = 2
    };
    QG_BlockModel(QObject * parent = nullptr);
    Qt::ItemFlags flags ( const QModelIndex & /*index*/ ) const override {
        return Qt::ItemIsSelectable|Qt::ItemIsEnabled;}
    int columnCount(const QModelIndex &/*parent*/) const  override {
        return static_cast<int>(LAST);
    }
    int rowCount ( const QModelIndex & parent = {} ) const override;
    QVariant data ( const QModelIndex & index, int role = Qt::DisplayRole ) const override;
    QModelIndex parent ( const QModelIndex & index ) const override;
    QModelIndex index ( int row, int column, const QModelIndex & parent = {} ) const override;
    void setBlockList(RS_BlockList* bl);
    RS_Block *getBlock( int row) const;
    QModelIndex getIndex (RS_Block * blk) const;

    RS_Block* getActiveBlock() const { return m_activeBlock; }
    void setActiveBlock(RS_Block* b) { m_activeBlock = b; }
private:
    QList<RS_Block*> m_listBlock;
    QIcon m_iconBlockVisible;
    QIcon m_iconBlockHidden;
    RS_Block* m_activeBlock {nullptr};
};


/**
 * This is the Qt implementation of a widget which can view a
 * block list.
 */
class QG_BlockWidget: public LC_GraphicViewAwareWidget, public RS_BlockListListener {
    Q_OBJECT
public:
    QG_BlockWidget(LC_ActionGroupManager* m_actionGroupManager, QG_ActionHandler* ah, QWidget* parent,
                   const char* name=nullptr, Qt::WindowFlags f = {});
    void setGraphicView(RS_GraphicView* doc) override;
    RS_BlockList* getBlockList() {
        return m_blockList;
    }
    void update();
    void activateBlock(RS_Block* block);
    void blockAdded(RS_Block*) override;
    void blockEdited(RS_Block*) override{
        update();
    }
    void blockRemoved(RS_Block*) override{
        update();
    }
    void blockToggled(RS_Block*) override{
        update();
    }
signals:
    void escape();
public slots:
    void slotActivated(const QModelIndex &blockIdx);
    void slotSelectionChanged(const QItemSelection &selected,const QItemSelection &deselected) const;
    void slotUpdateBlockList() const;
    void updateWidgetSettings() const;
protected:
    void contextMenuEvent(QContextMenuEvent *e) override;
    void addMenuItem(QMenu* contextMenu, RS2::ActionType actionType);
    void keyPressEvent(QKeyEvent* e) override;
    void setBlockList(RS_BlockList* blockList);
    void addToolbarButton(LC_FlexLayout* layButtons, RS2::ActionType actionType);
private:
    void restoreSelections() const;
    RS_BlockList* m_blockList = nullptr;
    QLineEdit* m_matchBlockName = nullptr;
    QTableView* m_blockView = nullptr;
    QG_BlockModel *m_blockModel = nullptr;
    QSortFilterProxyModel* m_proxyModel = nullptr;
    RS_Block* m_lastBlock = nullptr;
    QG_ActionHandler* m_actionHandler = nullptr;
    LC_ActionGroupManager* m_actionGroupManager{nullptr};
};

#endif
