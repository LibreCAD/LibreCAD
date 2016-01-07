/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2010 R. van Twisk (librecad@rvt.dds.nl)
** Copyright (C) 2014 Dongxu Li (dongxuli2011@gmail.com)
** Copyright (C) 2014 Pavel Krejcir (pavel@pamsoft.cz)

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

#ifndef LC_ACTIONDRAWSPLINEPOINTS_H
#define LC_ACTIONDRAWSPLINEPOINTS_H

#include "rs_previewactioninterface.h"
#include "rs_actiondrawspline.h"

/**
 * This action class can handle user events to draw splines through points.
 *
 * @author Pavel Krejcir
 */
class LC_ActionDrawSplinePoints : public RS_ActionDrawSpline
{
	Q_OBJECT
public:
	/**
	* Action States.
	*/
	enum Status
	{
		SetStartPoint,   /**< Setting the startpoint.  */
		SetNextPoint      /**< Setting the next point. */
	};

    LC_ActionDrawSplinePoints(RS_EntityContainer& container,
		RS_GraphicView& graphicView);
    ~LC_ActionDrawSplinePoints();

	void reset();

	virtual void init(int status = 0);
	virtual void trigger();

	virtual void mouseMoveEvent(QMouseEvent* e);
	virtual void mouseReleaseEvent(QMouseEvent* e);

	virtual void coordinateEvent(RS_CoordinateEvent* e);
	virtual void commandEvent(RS_CommandEvent* e);
	virtual QStringList getAvailableCommands();

	virtual void showOptions();
	virtual void hideOptions();

	virtual void updateMouseButtonHints();
	virtual void updateMouseCursor();

    virtual void setClosed(bool c);
    virtual bool isClosed();
    virtual void undo();
    //using degree=2 only
    virtual void setDegree(int /*deg*/){}

private:
	void redo();

	struct Points;
	std::unique_ptr<Points> pPoints;

};

#endif

