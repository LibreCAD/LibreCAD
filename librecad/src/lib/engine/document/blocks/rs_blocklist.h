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

#ifndef RS_BLOCKLIST_H
#define RS_BLOCKLIST_H

#include <cstddef>

#include <QList>

#include "lc_trackedlistener.h"

class QString;
class RS_Block;
class RS_BlockListListener;

/**
 * List of blocks.
 *
 * @see RS_Block
 *
 * @author Andrew Mustun
 */
class RS_BlockList {
public:
    explicit RS_BlockList(bool owner = false);
    virtual ~RS_BlockList();
    RS_BlockList(const RS_BlockList&) = delete;
    RS_BlockList& operator=(const RS_BlockList&) = delete;

    void clear();
    /**
     * @return Number of blocks available.
     */
    int count() const;

    /**
     * @return Block at given position or NULL if i is out of range.
     */
    RS_Block* at(int i);
    RS_Block* at(int i) const;
    //! \{ \brief range based loop
    QList<RS_Block*>::iterator begin();
    QList<RS_Block*>::iterator end();
    QList<RS_Block*>::const_iterator begin() const;
    QList<RS_Block*>::const_iterator end() const;
    //! \}

    void activate(const QString& name);
    void activate(RS_Block* block);
    //! @return The active block of NULL if no block is activated.
    RS_Block* getActive() const;

    bool add(RS_Block* block, bool notify = true);
    void addNotification();
    void remove(RS_Block* block);
    bool rename(RS_Block* block, const QString& name);
    //virtual void editBlock(RS_Block* block, const RS_Block& source);
    RS_Block* find(const QString& name);
    const RS_Block* find(const QString& name) const;
    RS_Block* findCaseInsensitive(const QString& name) const;
    QString newName(const QString& suggestion = "");
    void toggle(const QString& name);
    void toggle(RS_Block* block);
    /** Toggles each distinct block once and emits one full-list notification. */
    bool toggleMulti(const QList<RS_Block*>& blocks);
    void freezeAll(bool freeze);

    void addListener(RS_BlockListListener* listener);
    void removeListener(RS_BlockListListener* listener);
    /// how many listeners are registered (a widget that is destroyed while attached must have unregistered)
    int listenerCount() const {return static_cast<int>(m_blockListListeners.size());}

    bool isOwner() const {
        return m_owner;
    }

    void setOwner(const bool ow) {
        m_owner = ow;
    }

    /**
     * Sets the block list modified status to 'm'.
     */
    void setModified(bool m);

    /**
     * @retval true The block list has been modified.
     * @retval false The block list has not been modified.
     */
    bool isModified() const;

    /**
     * Changes whenever name-to-block resolution can change. Values are unique
     * across all block lists, so a (list, generation) pair cached by an
     * RS_Insert never matches a later list that reuses a freed list's address.
     */
    [[nodiscard]] std::size_t generation() const noexcept {
        return m_generation;
    }

    friend std::ostream& operator <<(std::ostream& os, RS_BlockList& b);

private:
    //! Is the list owning the blocks?
    bool m_owner = false;
    //! Blocks in the graphic
    QList<RS_Block*> m_blocks;
    //! List of registered BlockListListeners
    LC_ListenerList<RS_BlockListListener, RS_BlockList> m_blockListListeners{this};
    //! Currently active block
    RS_Block* m_activeBlock = nullptr;
    /** Flag set if the block list was modified and not yet saved. */
    bool m_modified = false;
    std::size_t m_generation = 0U;
};

#endif
