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

#include "rs_line.h"
#include "rs_vector.h"
#include "qg_actionhandler.h"
#include <memory>
#include "rs_actioninterface.h"
#include "lc_highlight.h"
#include "rs_arc.h"
#include "rs_ellipse.h"
#include "lc_defaults.h"
#include "lc_refellipse.h"

struct RS_CircleData;
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
    void init(int status) override;
    void finish(bool updateTB = true) override;
    void suspend() override;
    void resume() override;
    void trigger() override;
protected:
    /**
     * Preview that holds the entities to be previewed.
     */
    std::unique_ptr<RS_Preview> preview;
    bool hasPreview = true;//whether preview is in use

    std::unique_ptr<LC_Highlight> highlight;

    double refPointSize = 2.0;
    int refPointMode = DXF_FORMAT_PDMode_EncloseSquare(DXF_FORMAT_PDMode_CentreDot);
    bool showRefEntitiesOnPreview = false;
    bool highlightEntitiesOnHover = false;
    bool highlightEntitiesRefPointsOnHover = false;

    void drawPreview();
    void deletePreview();

    void deleteHighlights();
    void drawHighlights();
    void addToHighlights(RS_Entity *e, bool enable = true);

    bool trySnapToRelZeroCoordinateEvent(const QMouseEvent *e);
    RS_Vector getRelZeroAwarePoint(const QMouseEvent *e, const RS_Vector &pos);
    RS_Vector getSnapAngleAwarePoint(const QMouseEvent *e, const RS_Vector &basepoint, const RS_Vector &pos, bool drawMark = false);
    RS_Vector getSnapAngleAwarePoint(const QMouseEvent *e, const RS_Vector &basepoint, const RS_Vector &pos, bool drawMark, bool force);
    RS_Vector getFreeSnapAwarePoint(const QMouseEvent *e, const RS_Vector &pos) const;
    RS_Vector getFreeSnapAwarePointAlt(const QMouseEvent *e, const RS_Vector &pos) const;

    void previewEntity(RS_Entity *en);
    RS_Circle* previewCircle(const RS_CircleData& circleData);
    RS_Arc *previewArc(const RS_ArcData &arcData);
    RS_Ellipse *previewEllipse(const RS_EllipseData &ellipseData);
    void previewPoint(const RS_Vector &coord);
    RS_Line* previewLine(const RS_Vector &start, const RS_Vector &end);
    RS_Line* previewLine(const RS_LineData &data);
    RS_Line* previewRefLine(const RS_Vector &start, const RS_Vector &end);
    void previewRefLines(const std::vector<RS_LineData>& points);
    void previewRefSelectableLine(const RS_Vector &start, const RS_Vector &end);
    void previewRefPoint(const RS_Vector &coord);
    void previewRefSelectablePoint(const RS_Vector &coord);
    void previewRefPoints(const std::vector<RS_Vector>& points);
    RS_Arc* previewRefArc(const RS_Vector &center, const RS_Vector &startPoint, const RS_Vector &mouse, bool determineReversal);
    RS_Circle* previewRefCircle(const RS_Vector &center, const double radius);
    RS_Arc *previewRefArc(const RS_ArcData &arcData);
    LC_RefEllipse *previewRefEllipse(const RS_EllipseData &arcData);
    void initRefEntitiesMetrics();
    void highlightHover(RS_Entity *e);
    void highlightHoverWithRefPoints(RS_Entity* e, bool value);
    void highlightSelected(RS_Entity *e, bool enable=true);
    virtual void moveRelativeZero(const RS_Vector &zero);
    bool is(RS_Entity* e, RS2::EntityType type) const;
    bool isLine(RS_Entity*  e) const{return is(e, RS2::EntityLine);};
    bool isPolyline(RS_Entity*  e) const{return is(e, RS2::EntityPolyline);};
    bool isCircle(RS_Entity*  e){return is(e, RS2::EntityCircle);};
    bool isArc(RS_Entity*  e){return is(e, RS2::EntityArc);};
    bool isEllipse(RS_Entity*  e){return is(e, RS2::EntityEllipse);};
    bool addToDocumentUndoable(RS_Undoable* e) const;
    void previewSnapAngleMark(const RS_Vector &center, const RS_Vector &refPoint);
    RS_Entity *catchModifiableEntity(QMouseEvent *e, const EntityTypeList &enTypeList);
    RS_Entity *catchModifiableEntity(QMouseEvent *e, const RS2::EntityType &enType);
    RS_Entity *catchModifiableEntity(RS_Vector &coord, const RS2::EntityType &enType);
    void deleteEntityUndoable(RS_Entity *entity);
    void previewSnapAngleMark(const RS_Vector &center, double angle);
};
#endif
