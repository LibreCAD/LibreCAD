
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
#ifndef RS_ACTIONPOLYLINEEQUIDISTANT_H
#define RS_ACTIONPOLYLINEEQUIDISTANT_H

#include "lc_undoabledocumentmodificationaction.h"
#include "rs_previewactioninterface.h"
class RS_Polyline;

/**
 * This action class can handle user events to move entities.
 *
 * @author Andrew Mustun
 */
class LC_ActionPolylineEquidistant:public LC_UndoableDocumentModificationAction {
    Q_OBJECT
public:
    explicit LC_ActionPolylineEquidistant(LC_ActionContext *actionContext);
    ~LC_ActionPolylineEquidistant() override;
    void init(int status) override;
    void setDistance(const double d){m_distance = d;}
    double getDistance() const{return m_distance;}
    void setCopiesNumber(const unsigned n){m_copiesNumber = n;}
    int getCopiesNumber() const{return m_copiesNumber;}
protected:
    /**
 * Action States.
 */
    enum Status {
        ChooseEntity = InitialActionStatus  /**< Choosing the original polyline. */
    };

    RS_Polyline *m_originalEntity = nullptr;
    double m_distance = 0.;
    int m_copiesNumber = 0;
    bool m_bRightSide = false;

    RS_Entity *calculateOffset(RS_Entity *newEntity, RS_Entity *orgEntity, double distance) const;
    RS_Vector calculateIntersection(const RS_Entity *first, const RS_Entity *last);
    void makeContour(const RS_Polyline* originalPolyline, bool contourOnRightSide, QList<RS_Polyline *> &createdPolylines);
    bool isPointOnRightSideOfPolyline(const RS_Polyline *polyline, const RS_Vector &snapPoint) const;
    RS2::CursorType doGetMouseCursor(int status) override;
    void onMouseLeftButtonRelease(int status, const LC_MouseEvent* e) override;
    void onMouseRightButtonRelease(int status, const LC_MouseEvent* e) override;
    void onMouseMoveEvent(int status, const LC_MouseEvent* e) override;
    void setPolylineToModify(const LC_MouseEvent* e, RS_Entity* en);
    void updateActionPrompt() override;
    LC_ActionOptionsWidget* createOptionsWidget() override;
    LC_ActionOptionsPropertiesFiller* createOptionsFiller() override;
    bool doTriggerModifications(LC_DocumentModificationBatch& ctx) override;
    void doTriggerCompletion(bool success) override;
    bool doUpdateDistanceByInteractiveInput(const QString& tag, double distance) override;
    void doSaveOptions() override;
    void doLoadOptions() override;
};
#endif
