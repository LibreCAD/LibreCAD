/****************************************************************************
**
 * Draw ellipse by foci and a point on ellipse

Copyright (C) 2012 Dongxu Li (dongxuli2011@gmail.com)
Copyright (C) 2011 R. van Twisk (librecad@rvt.dds.nl)

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
**********************************************************************/

#ifndef RS_ACTIONDRAWCIRCLETAN2_1P_H
#define RS_ACTIONDRAWCIRCLETAN2_1P_H

#include "rs_previewactioninterface.h"

class RS_AtomicEntity;
struct RS_CircleData;

/**
 * Given two circles and a point, draw a common tangent circle passing the point
 *
 * @author Dongxu Li
 */
class RS_ActionDrawCircleTan2_1P : public RS_PreviewActionInterface {
        Q_OBJECT
public:
    /**
     * Action States.
     */
    enum Status {
        SetCircle1=0,   //  Setting the First Circle.  */
        SetCircle2=1,   //  Setting the First Circle.  */
        SetPoint=2,   //  Setting the Second Circle.  */
        SetCenter
    };

public:
    RS_ActionDrawCircleTan2_1P(RS_EntityContainer& container,
                                 RS_GraphicView& graphicView);
	~RS_ActionDrawCircleTan2_1P();

    virtual void init(int status=0);

    virtual void trigger();
    virtual bool getCenters();
    virtual bool preparePreview();

    virtual void mouseMoveEvent(QMouseEvent* e);
    virtual void mouseReleaseEvent(QMouseEvent* e);

	virtual void coordinateEvent(RS_CoordinateEvent* e);
//        virtual void commandEvent(RS_CommandEvent* e);
    virtual QStringList getAvailableCommands();
    virtual void finish(bool updateTB=true);
    virtual void updateMouseButtonHints();
	virtual void updateMouseCursor();


protected:
    RS_Entity* catchCircle(QMouseEvent* e);
	std::vector<RS_AtomicEntity*> circles;
private:
	struct Points;
	std::unique_ptr<Points> pPoints;
};

#endif
