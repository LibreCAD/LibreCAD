
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

#include "rs_previewactioninterface.h"
class RS_Polyline;

/**
 * This action class can handle user events to move entities.
 *
 * @author Andrew Mustun
 */
class RS_ActionPolylineEquidistant:public RS_PreviewActionInterface {
    Q_OBJECT
public:
    RS_ActionPolylineEquidistant(LC_ActionContext *actionContext);
    ~RS_ActionPolylineEquidistant() override;
    void init(int status) override;
    void setDist(const double &d){m_dist = d;}
    double getDist() const{return m_dist;}
    void setNumber(unsigned n){m_number = n;}
    int getNumber() const{return m_number;}
protected:
    /**
 * Action States.
 */
    enum Status {
        ChooseEntity = InitialActionStatus  /**< Choosing the original polyline. */
    };

    RS_Polyline *m_originalEntity = nullptr;
    double m_dist = 0.;
    int m_number = 0;
    bool m_bRightSide = false;

    RS_Entity *calculateOffset(RS_Entity *newEntity, RS_Entity *orgEntity, double dist);
    RS_Vector calculateIntersection(RS_Entity *first, RS_Entity *last);
    void makeContour(RS_Polyline *originalPolyline, bool contourOnRightSide, QList<RS_Polyline *> &createdPolylines);
    bool isPointOnRightSideOfPolyline(const RS_Polyline *polyline, const RS_Vector &snapPoint) const;
    RS2::CursorType doGetMouseCursor(int status) override;
    void onMouseLeftButtonRelease(int status, LC_MouseEvent *e) override;
    void onMouseRightButtonRelease(int status, LC_MouseEvent *e) override;
    void onMouseMoveEvent(int status, LC_MouseEvent *event) override;
    void setPolylineToModify(LC_MouseEvent* e, RS_Entity* en);
    void updateMouseButtonHints() override;
    LC_ActionOptionsWidget* createOptionsWidget() override;
    void doTrigger() override;
    bool doUpdateDistanceByInteractiveInput(const QString& tag, double distance) override;
};
#endif
