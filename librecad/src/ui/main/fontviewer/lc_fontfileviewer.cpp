/*
 * ********************************************************************************
 * This file is part of the LibreCAD project, a 2D CAD program
 *
 * Copyright (C) 2025 LibreCAD.org
 * Copyright (C) 2025 sand1024
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 * ********************************************************************************
 */

#ifndef LC_FontFileViewer_INCUDE_H
#define LC_FontFileViewer_INCUDE_H
#include "lc_fontfileviewer.h"

#include "rs_block.h"
#include "rs_blocklist.h"
#include "rs_document.h"
#include "rs_graphic.h"
#include "rs_insert.h"
#include "rs_mtext.h"
#include "rs_settings.h"

LC_FontFileViewer::LC_FontFileViewer(RS_Document* doc): m_document{doc} {
}

void LC_FontFileViewer::drawFontChars() {
    RS_BlockList* bl = m_document->getBlockList();
    double sep = m_document->getGraphic()->getVariableDouble("LetterSpacing", 3.0);
    double h = sep/3;
    sep = sep*3;
    int columnCount = LC_GET_ONE_INT("Render", "FontLettersColumnsCount", 10);
    if (columnCount == 0) {
        columnCount = INT_MAX;
    }
    int currentColumn = 0;
    int currentRow = 0;

    int currentLetterY = 0;
    double maxLetterHeight = 0;
    double maxLetterWidth  = 0;

    for (int i=0; i<bl->count(); ++i) {
        RS_Block* ch = bl->at(i);
        maxLetterHeight = std::max(maxLetterHeight, ch->getSize().y);
        maxLetterWidth = std::max(maxLetterWidth, ch->getSize().x);
    }

    double currentLetterX  = 0;

    for (int i=0; i<bl->count(); ++i) {
        RS_Block* ch = bl->at(i);
        RS_InsertData data(ch->getName(), RS_Vector(currentColumn*sep,currentLetterY), RS_Vector(1,1), 0, 1, 1, RS_Vector(0,0));
        currentLetterX += maxLetterWidth + sep;
        auto in = new RS_Insert(m_document, data);
        m_document->addEntity(in);
        QString uCode = (ch->getName()).mid(1,4);
        RS_MTextData datatx(RS_Vector(currentColumn*sep,currentLetterY-h), h, 4*h, RS_MTextData::VATop,
                           RS_MTextData::HALeft, RS_MTextData::ByStyle, RS_MTextData::AtLeast,
                           1, uCode, "standard", 0);
        auto tx = new RS_MText(m_document, datatx);
        m_document->addEntity(tx);

        currentColumn ++;
        if (currentColumn == columnCount) {
            currentColumn = 0;
            currentLetterX = 0;
            currentRow ++;
            currentLetterY -= maxLetterHeight*1.5;
        }
    }
}

#endif // LC_FontFileViewer_INCUDE_H
