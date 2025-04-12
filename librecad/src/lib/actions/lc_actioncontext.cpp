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

#include "lc_actioncontext.h"
#include "rs_document.h"

RS_EntityContainer * LC_ActionContext::getEntityContainer(){
    return m_entityContainer;
}

RS_GraphicView * LC_ActionContext::getGraphicView(){
    return m_graphicView;
}

void LC_ActionContext::setDocumentAndView(RS_Document *document, RS_GraphicView *view){
        m_graphicView = view;
        m_entityContainer = document;
}
