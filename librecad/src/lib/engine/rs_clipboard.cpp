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

#include <iostream>

#include "rs_block.h"
#include "rs_clipboard.h"
#include "rs_entity.h"
#include "rs_graphic.h"
#include "rs_layer.h"


RS_Clipboard::RS_Clipboard():
    m_graphic{std::make_unique<RS_Graphic>()}
{
}

RS_Clipboard* RS_Clipboard::instance() {
    struct RS_ClipboardMaker : public RS_Clipboard {
        using RS_Clipboard::RS_Clipboard;
    };

    static std::unique_ptr<RS_Clipboard> uniqueInstance = std::make_unique<RS_ClipboardMaker>();
    assert(uniqueInstance != nullptr);
    return uniqueInstance.get();
}

void RS_Clipboard::clear() {
    m_graphic->clear();
    m_graphic->clearBlocks();
    m_graphic->clearLayers();
    m_graphic->clearVariables();
}

void RS_Clipboard::addBlock(RS_Block* b) {
    if (b) {
        m_graphic->addBlock(b, false);
    }
}

bool RS_Clipboard::hasBlock(const QString& name) {
    return (m_graphic->findBlock(name));
}

void RS_Clipboard::addLayer(RS_Layer* l) {
    if (l) {
        //m_graphic->addLayer(l->clone());
        m_graphic->addLayer(l);
    }
}

bool RS_Clipboard::hasLayer(const QString& name) {
    return (m_graphic->findLayer(name));
}

void RS_Clipboard::addEntity(RS_Entity* e) {
    if (e) {
        //m_graphic->addEntity(e->clone());
        m_graphic->addEntity(e);
        e->reparent(m_graphic.get());
    }
}

int  RS_Clipboard::countBlocks() {
    return m_graphic->countBlocks();
}
RS_Block* RS_Clipboard::blockAt(int i) {
    return m_graphic->blockAt(i);
}

int  RS_Clipboard::countLayers() {
    return m_graphic->countLayers();
}

RS_Layer* RS_Clipboard::layerAt(int i) {
    return m_graphic->layerAt(i);
}

unsigned RS_Clipboard::count() {
    return m_graphic->count();
}

RS_Entity* RS_Clipboard::entityAt(unsigned i) {
    return m_graphic->entityAt(i);
}

RS_Entity* RS_Clipboard::firstEntity() {
    return m_graphic->firstEntity();
}

RS_Entity* RS_Clipboard::nextEntity() {
    return m_graphic->nextEntity();
}

RS_Graphic* RS_Clipboard::getGraphic() {
    return m_graphic.get();
}

/**
 * Dumps the clipboard contents to stdout.
 */
std::ostream& operator << (std::ostream& os, RS_Clipboard& cb) {
    os << "Clipboard: " << *cb.m_graphic << "\n";

    return os;
}

