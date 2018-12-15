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


#ifndef RS_BLOCKLISTLISTENER_H
#define RS_BLOCKLISTLISTENER_H

#include "rs_block.h"

/**
 * This class is an interface for classes that are interested in
 * knowing about changes in the block list. 
 */
class RS_BlockListListener {
public:
    RS_BlockListListener() {}
    virtual ~RS_BlockListListener() {}

    /**
     * Called when the active block changes.
     */
    virtual void blockActivated(RS_Block*) {}

    /**
     * Called when a new block is added to the list.
     */
    virtual void blockAdded(RS_Block*) {}

    /**
     * Called when a block is removed from the list.
     */
    virtual void blockRemoved(RS_Block*) {}

    /**
     * Called when a block's attributes are modified.
     */
    virtual void blockEdited(RS_Block*) {}

    /**
     * Called when a block's visibility is toggled. 
     */
    virtual void blockToggled(RS_Block*) {}

    /**
     * Called when block list is modified.
     */
    virtual void blockListModified(bool) {}
}
;

#endif
