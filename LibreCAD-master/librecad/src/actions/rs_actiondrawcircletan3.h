/****************************************************************************
**
 * Draw a common tangent circle of 3 existing circles
 * Problem of Appollonius

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

#ifndef RS_ACTIONDRAWCIRCLETAN3_H
#define RS_ACTIONDRAWCIRCLETAN3_H

#include<vector>
#include "rs_previewactioninterface.h"

struct RS_CircleData;
class RS_AtomicEntity;

/**
 * Draw Common tangential circle of 3 given circles, i.e. Appollonius's problem
 *
 * @author Dongxu Li
 */
class RS_ActionDrawCircleTan3 : public RS_PreviewActionInterface {
        Q_OBJECT
public:
    /**
     * Action States.
     */
    enum Status {
        SetCircle1,   //  Setting the First Circle.  */
        SetCircle2,   //  Setting the Second Circle.  */
        SetCircle3,   //  Setting the Third Circle.  */
        SetCenter   //  select the closest tangential Circle.  */
    };

public:
    RS_ActionDrawCircleTan3(RS_EntityContainer& container,
                                 RS_GraphicView& graphicView);
	~RS_ActionDrawCircleTan3() override;

	void init(int status=0) override;

	void trigger() override;
	bool getData();
	bool preparePreview();

	void mouseMoveEvent(QMouseEvent* e) override;
	void mouseReleaseEvent(QMouseEvent* e) override;

	//        void coordinateEvent(RS_CoordinateEvent* e) override;
	//    void commandEvent(RS_CommandEvent* e) override;
	QStringList getAvailableCommands() override;
	void finish(bool updateTB=true) override;
	void updateMouseButtonHints() override;
	void updateMouseCursor() override;

//protected:
    private:
	std::vector<double> verifyCenter(const RS_Vector& center) const;
	std::vector<double> getRadii(RS_AtomicEntity* entity, const RS_Vector& center) const;
    RS_Entity* catchCircle(QMouseEvent* e);

	struct Points;
	std::unique_ptr<Points> pPoints;

};

#endif
