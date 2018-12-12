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

#include "qc_graphicview.h"


#include "rs_actiondefault.h"
#include "rs_settings.h"

#include "qc_applicationwindow.h"
#include "rs_debug.h"

QC_GraphicView::QC_GraphicView(RS_Document* doc, QWidget* parent)
        :QG_GraphicView(parent, "graphicview") {

    RS_DEBUG->print("QC_GraphicView::QC_GraphicView()..");

    RS_DEBUG->print("  Setting Container..");
    if (doc) {
        setContainer(doc);
        doc->setGraphicView(this);
    }
    RS_DEBUG->print("  container set.");
    setFactorX(4.0);
    setFactorY(4.0);
    setOffset(50, 50);
    setBorders(10, 10, 10, 10);
	
	if (doc) {
		setDefaultAction(new RS_ActionDefault(*doc, *this));
	}
}

// EOF
