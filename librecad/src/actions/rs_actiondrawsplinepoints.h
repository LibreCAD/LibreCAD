/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2010 R. van Twisk (librecad@rvt.dds.nl)
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

#ifndef RS_ACTIONDRAWSPLINEPOINTS_H
#define RS_ACTIONDRAWSPLINEPOINTS_H

#include "rs_previewactioninterface.h"
#include "rs_splinepoints.h"

/**
 * This action class can handle user events to draw splines through points.
 *
 * @author Pavel Krejcir
 */
class LC_ActionDrawSplinePoints : public RS_PreviewActionInterface
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
    virtual ~LC_ActionDrawSplinePoints();

	virtual RS2::ActionType rtti()
	{
		return RS2::ActionDrawSplinePoints;
	}

	static QAction* createGUIAction(RS2::ActionType /*type*/, QObject* /*parent*/);

	void clear();
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
	//virtual void updateToolBar();

	//void close();
private:
	/**
	* Spline data defined so far.
	*/
    LC_SplinePointsData data;

	/**
	* Spline entity we're working on.
	*/
    LC_SplinePoints* spline;

	/**
	* Point history (for undo)
	*/
	QList<RS_Vector> undoBuffer;

	void undo();
	void redo();
};

#endif

