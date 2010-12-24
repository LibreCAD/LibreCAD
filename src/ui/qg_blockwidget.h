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

#ifndef QG_BLOCKWIDGET_H
#define QG_BLOCKWIDGET_H

#include <qwidget.h>
#include <q3listbox.h>
//Added by qt3to4:
#include <QKeyEvent>
#include <QContextMenuEvent>
#include <QPixmap>

#include "rs_blocklist.h"
#include "rs_blocklistlistener.h"

#include "qg_actionhandler.h"

/**
 * This is the Qt implementation of a widget which can view a 
 * block list.
 */
class QG_BlockWidget: public QWidget, public RS_BlockListListener {
    Q_OBJECT

public:
    QG_BlockWidget(QG_ActionHandler* ah, QWidget* parent,
                   const char* name=0, Qt::WFlags f = 0);
    ~QG_BlockWidget();

    void setBlockList(RS_BlockList* blockList) {
        this->blockList = blockList;
        update();
    }

    RS_BlockList* getBlockList() {
        return blockList;
    }

    void update();
    void highlightBlock(RS_Block* block);

    //virtual void blockActivated(RS_Block *) {}
    virtual void blockAdded(RS_Block*) {
        update();
    }
    virtual void blockEdited(RS_Block*) {
        update();
    }
    virtual void blockRemoved(RS_Block*) {
        update();
    }
    virtual void blockToggled(RS_Block*) {
		update();
	}

signals:
	void escape();

public slots:
    //void slotToggleView(QListBoxItem* item);
    void slotActivated(const QString& blockName);
	void slotMouseButtonClicked(int button, Q3ListBoxItem* item, 
		const QPoint& pos);

protected:
    void contextMenuEvent(QContextMenuEvent *e);
	virtual void keyPressEvent(QKeyEvent* e);

private:
    RS_BlockList* blockList;
    Q3ListBox* listBox;
	RS_Block* lastBlock;
    QPixmap pxmVisible;
    QPixmap pxmHidden;
    QPixmap pxmAdd;
    QPixmap pxmRemove;
    QPixmap pxmAttributes;
    QPixmap pxmEdit;
    QPixmap pxmInsert;
    QPixmap pxmDefreezeAll;
    QPixmap pxmFreezeAll;
    QG_ActionHandler* actionHandler;
};

#endif
