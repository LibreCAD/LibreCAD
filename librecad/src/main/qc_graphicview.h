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

#ifndef QC_GRAPHICVIEW_H
#define QC_GRAPHICVIEW_H

#include <QWidget>

#include "rs_document.h"
#include "rs_eventhandler.h"

#include "qg_graphicview.h"

class QC_ApplicationWindow;

/**
 * A view widget for the visualisation of drawings.
 * Very thin wrapper for LibreCAD specific settings.
 *
 * @author Andrew Mustun
 */
class QC_GraphicView : public QG_GraphicView {
    Q_OBJECT

public:
    QC_GraphicView(RS_Document* doc, QWidget* parent=0);
	virtual ~QC_GraphicView()=default;

private:
    //RS_Document* document;
};

#endif

