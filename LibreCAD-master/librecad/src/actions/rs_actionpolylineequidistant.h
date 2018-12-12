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

/**
 * This action class can handle user events to move entities.
 *
 * @author Andrew Mustun
 */
class RS_ActionPolylineEquidistant : public RS_PreviewActionInterface {
	Q_OBJECT
public:
	/**
	 * Action States.
	 */
	enum Status {
		ChooseEntity			/**< Choosing the original polyline. */
	};

public:
	RS_ActionPolylineEquidistant(RS_EntityContainer& container,
						RS_GraphicView& graphicView);
	~RS_ActionPolylineEquidistant() override;

	void init(int status=0) override;
	
	void trigger() override;

//        void mouseMoveEvent(QMouseEvent* e) override;
		void mouseReleaseEvent(QMouseEvent* e) override;
	
	void updateMouseButtonHints() override;
	void updateMouseCursor() override;
//	void updateToolBar() override;
	void showOptions() override;
	void hideOptions() override;

    void setDist(const double& d) {
		dist = d;
	}

	double getDist() const{
		return dist;
	}

    void setNumber(unsigned n) {
		number = n;
	}

	int getNumber() const{
		return number;
	}

	bool makeContour();

private:
    RS_Entity* calculateOffset(RS_Entity* newEntity,RS_Entity* orgEntity, double dist);
    RS_Vector calculateIntersection(RS_Entity* first,RS_Entity* last);

private:
	RS_Entity* originalEntity;
	std::unique_ptr<RS_Vector> targetPoint;
	double dist;
	int number;
	bool bRightSide;
};

#endif
