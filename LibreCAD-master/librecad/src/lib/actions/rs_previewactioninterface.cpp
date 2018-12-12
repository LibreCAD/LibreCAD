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


#include "rs_previewactioninterface.h"

#include "rs_graphicview.h"
#include "rs_preview.h"
#include "rs_debug.h"

/**
 * Constructor.
 *
 * Sets the entity container on which the action class inherited
 * from this interface operates.
 */
RS_PreviewActionInterface::RS_PreviewActionInterface(const char* name,
													 RS_EntityContainer& container,
													 RS_GraphicView& graphicView) :
	RS_ActionInterface(name, container, graphicView)
  ,preview(new RS_Preview(&container))
//  ,offset(new RS_Vector{})
{

    RS_DEBUG->print("RS_PreviewActionInterface::RS_PreviewActionInterface: Setting up action with preview: \"%s\"", name);

    // preview is linked to the container for getting access to
    //   document settings / dictionary variables

    preview->setLayer(NULL);
    hasPreview = true;

    RS_DEBUG->print("RS_PreviewActionInterface::RS_PreviewActionInterface: Setting up action with preview: \"%s\": OK", name);
}



/** Destructor */
RS_PreviewActionInterface::~RS_PreviewActionInterface() {
    deletePreview();
}



void RS_PreviewActionInterface::init(int status) {
    deletePreview();
    RS_ActionInterface::init(status);
}



void RS_PreviewActionInterface::finish(bool updateTB) {
    deletePreview();
    RS_ActionInterface::finish(updateTB);
}



void RS_PreviewActionInterface::suspend() {
    RS_ActionInterface::suspend();
    deletePreview();
}



void RS_PreviewActionInterface::resume() {
    RS_ActionInterface::resume();
    drawPreview();
}



void RS_PreviewActionInterface::trigger() {
    RS_ActionInterface::trigger();
    deletePreview();
}


/**
 * Deletes the preview from the screen.
 */
void RS_PreviewActionInterface::deletePreview() {
		if (hasPreview){
                //avoid deletiong NULL or empty preview
            preview->clear();
            hasPreview=false;
        }
	if(!graphicView->isCleanUp()){
		graphicView->getOverlayContainer(RS2::ActionPreviewEntity)->clear();
	}
}



/**
 * Draws / deletes the current preview.
 */
void RS_PreviewActionInterface::drawPreview() {
	// RVT_PORT How does offset work??        painter->setOffset(offset);
	RS_EntityContainer *container=graphicView->getOverlayContainer(RS2::ActionPreviewEntity);
	container->clear();
	container->setOwner(false); // Little hack for now so we don't delete the preview twice
	container->addEntity(preview.get());
	graphicView->redraw(RS2::RedrawOverlay);
	hasPreview=true;
}

