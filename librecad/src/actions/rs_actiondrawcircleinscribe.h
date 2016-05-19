/****************************************************************************
**
 * Draw ellipse by foci and a point on ellipse

Copyright (C) 2011 Dongxu Li (dongxuli2011@gmail.com)
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

#ifndef RS_ACTIONDRAWCIRCLEINSCRIBE_H
#define RS_ACTIONDRAWCIRCLEINSCRIBE_H

#include "rs_previewactioninterface.h"

class RS_Line;
struct RS_CircleData;

/**
 * Draw ellipse by foci and a point on ellipse
 *
 * @author Dongxu Li
 */
class RS_ActionDrawCircleInscribe : public RS_PreviewActionInterface {
        Q_OBJECT
public:
    /**
     * Action States.
     */
    enum Status {
		SetLine1,   //  Setting the First Line.  */
        SetLine2,   //  Setting the Second Line.  */
        SetLine3   //  Setting the Third Line.  */
    };

public:
    RS_ActionDrawCircleInscribe(RS_EntityContainer& container,
                                 RS_GraphicView& graphicView);
	~RS_ActionDrawCircleInscribe() override;

	void init(int status=0) override;

	void trigger() override;
	bool preparePreview();

	void mouseMoveEvent(QMouseEvent* e) override;
	void mouseReleaseEvent(QMouseEvent* e) override;

	//        void coordinateEvent(RS_CoordinateEvent* e) override;
	//    void commandEvent(RS_CommandEvent* e) override;
	QStringList getAvailableCommands() override;
	void finish(bool updateTB=true) override;
	void updateMouseButtonHints() override;
	void updateMouseCursor() override;

private:
	/**
	 * @brief clearLines unset highlighten lines, and clear the vector "lines"
	 * @param checkStatus keep lines members according to getStatus()
	 */
	void clearLines(bool checkStatus=false);
	struct Points;
	std::unique_ptr<Points> pPoints;
    bool valid;
};

#endif
