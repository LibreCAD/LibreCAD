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

#ifndef RS_ACTIONDRAWCIRCLETAN1_2P_H
#define RS_ACTIONDRAWCIRCLETAN1_2P_H

#include "rs_previewactioninterface.h"

class RS_AtomicEntity;
struct RS_CircleData;

/**
 * Draw tangential circle passing 2 points
 *
 * @author Dongxu Li
 */
class RS_ActionDrawCircleTan1_2P : public RS_PreviewActionInterface {
        Q_OBJECT
    /**
     * Action States.
     */
    enum Status {
        SetCircle1=0,   //  Setting the First Circle.  */
        SetPoint1=1,   //  Setting the First Point.  */
        SetPoint2=2,   //  Setting the Second Point.  */
        SetCenter   //  Setting the internal or external tangent circle's center.  */
    };

public:
    RS_ActionDrawCircleTan1_2P(RS_EntityContainer& container,
                                 RS_GraphicView& graphicView);
	~RS_ActionDrawCircleTan1_2P() override;

	void init(int status=0) override;

	void trigger() override;
	bool getCenters();
	bool preparePreview();

	void mouseMoveEvent(QMouseEvent* e) override;
	void mouseReleaseEvent(QMouseEvent* e) override;

			void coordinateEvent(RS_CoordinateEvent* e) override;
//        void commandEvent(RS_CommandEvent* e) override;
	QStringList getAvailableCommands() override;
	void finish(bool updateTB=true) override;
	void updateMouseButtonHints() override;
	void updateMouseCursor() override;

//    void showOptions() override;
//    void hideOptions() override;
//    void setRadius(const double& r);
	double getRadius() const;


protected:
    RS_Entity* catchCircle(QMouseEvent* e);
	RS_AtomicEntity* circle;

private:
	struct Points;
	std::unique_ptr<Points> pPoints;
};

#endif
