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

#include <memory>
#include "rs_actioninterface.h"
#include "lc_highlight.h"

/**
 * This is the interface that must be implemented for all
 * action classes which need a preview.
 *
 * @author Andrew Mustun
 */
class RS_PreviewActionInterface : public RS_ActionInterface {
public:
    RS_PreviewActionInterface(
        const char *name,
        RS_EntityContainer &container,
        RS_GraphicView &graphicView,
        RS2::ActionType actionType = RS2::ActionNone);
    ~RS_PreviewActionInterface() override;
    void init(int status = 0) override;
    void finish(bool updateTB = true) override;
    void suspend() override;
    void resume() override;
    void trigger() override;
    void drawPreview();
    void deletePreview();

    void deleteHighlights();
    void drawHighlights();
    void addToHighlights(RS_Entity *e, bool enable = true);

private:

protected:
    /**
     * Preview that holds the entities to be previewed.
     */
    std::unique_ptr<RS_Preview> preview;
    bool hasPreview = true;//whether preview is in use

    std::unique_ptr<LC_Highlight> highlight;
//    /**
//     * Current offset of the preview.
//     */
//	std::unique_ptr<RS_Vector> offset;
    bool trySnapToRelZeroCoordinateEvent(const QMouseEvent *e);
    RS_Vector getRelZeroAwarePoint(const QMouseEvent *e, const RS_Vector &pos);
    void addReferencePointToPreview(const RS_Vector &coord);
    void addPointToPreview(const RS_Vector &coord);
    RS_Vector getSnapAngleAwarePoint(const QMouseEvent *e, const RS_Vector &basepoint, const RS_Vector &pos);
    void addReferenceLineToPreview(const RS_Vector &start, const RS_Vector &end);
    void addLineToPreview(const RS_Vector &start, const RS_Vector &end);
    void addReferenceArcToPreview(const RS_Vector &center, const RS_Vector &startPoint, const RS_Vector &mouse, bool determineReversal);
};

#endif
