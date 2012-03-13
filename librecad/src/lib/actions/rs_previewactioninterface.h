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


#ifndef RS_PREVIEWACTIONINTERFACE_H
#define RS_PREVIEWACTIONINTERFACE_H

#include "rs_actioninterface.h"
#include "rs_painter.h"
#include "rs_preview.h"

/**
 * This is the interface that must be implemented for all
 * action classes which need a preview.
 *
 * @author Andrew Mustun
 */
class RS_PreviewActionInterface : public RS_ActionInterface {
public:
    RS_PreviewActionInterface(const char* name,
                              RS_EntityContainer& container,
                              RS_GraphicView& graphicView);
    virtual ~RS_PreviewActionInterface();

    virtual void init(int status=0);
    virtual void finish(bool updateTB=true);
    virtual void suspend();
    virtual void resume();
    virtual void trigger();
    void drawPreview();
    void deletePreview();

private:

protected:
    /**
     * Preview that holds the entities to be previewed.
     */
    RS_Preview* preview;
    bool hasPreview;//whether preview is in use
    /**
     * Current offset of the preview.
     */
    RS_Vector offset;
};

#endif
