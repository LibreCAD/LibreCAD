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

#include "rs_clipboard.h"

#include <iostream>

#include "rs_graphic.h"

RS_Clipboard::RS_Clipboard() : m_graphic{std::make_unique<RS_Graphic>()} {
}

RS_Clipboard* RS_Clipboard::instance() {
    struct RS_ClipboardMaker : RS_Clipboard {
        using RS_Clipboard::RS_Clipboard;
    };

    static std::unique_ptr<RS_Clipboard> uniqueInstance = std::make_unique<RS_ClipboardMaker>();
    Q_ASSERT(uniqueInstance != nullptr);
    return uniqueInstance.get();
}

void RS_Clipboard::startCopy() const {
    m_graphic->setAutoUpdateBorders(false);
}

void RS_Clipboard::endCopy() const {
    m_graphic->setAutoUpdateBorders(true);
    m_graphic->calculateBorders();
}

void RS_Clipboard::clear() const {
    m_graphic->clear();
    m_graphic->clearBlocks();
    m_graphic->clearLayers();
    m_graphic->clearVariables();
}

void RS_Clipboard::addBlock(RS_Block* b) const {
    if (b != nullptr) {
        m_graphic->addBlock(b, false);
    }
}

bool RS_Clipboard::hasBlock(const QString& name) const {
    return m_graphic->findBlock(name) != nullptr;
}

void RS_Clipboard::addLayer(RS_Layer* l) const {
    if (l != nullptr) {
        m_graphic->addLayer(l);
    }
}

bool RS_Clipboard::hasLayer(const QString& name) const {
    return m_graphic->findLayer(name) != nullptr;
}

void RS_Clipboard::addEntity(RS_Entity* e) const {
    if (e != nullptr) {
        m_graphic->addEntity(e);
        e->reparent(m_graphic.get());
    }
}

int RS_Clipboard::countBlocks() const {
    return m_graphic->countBlocks();
}

RS_Block* RS_Clipboard::blockAt(const int i) const {
    return m_graphic->blockAt(i);
}

int RS_Clipboard::countLayers() const {
    return m_graphic->countLayers();
}

RS_Layer* RS_Clipboard::layerAt(const int i) const {
    return m_graphic->layerAt(i);
}

unsigned RS_Clipboard::count() const {
    return m_graphic->count();
}

RS_Entity* RS_Clipboard::entityAt(const unsigned i) const {
    return m_graphic->entityAt(i);
}

RS_Entity* RS_Clipboard::firstEntity() const {
    return m_graphic->firstEntity();
}

RS_Entity* RS_Clipboard::nextEntity() const {
    return m_graphic->nextEntity();
}

RS_Graphic* RS_Clipboard::getGraphic() const {
    return m_graphic.get();
}

/**
 * Dumps the clipboard contents to stdout.
 */
std::ostream& operator <<(std::ostream& os, const RS_Clipboard& cb) {
    os << "Clipboard: " << *cb.m_graphic << "\n";
    return os;
}
