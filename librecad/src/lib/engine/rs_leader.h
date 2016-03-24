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


#ifndef RS_LEADER_H
#define RS_LEADER_H

#include "rs_entity.h"
#include "rs_entitycontainer.h"



/**
 * Holds the data that defines a leader.
 */
class RS_LeaderData {
public:
	RS_LeaderData() = default;
	RS_LeaderData(bool arrowHeadFlag) {
		arrowHead = arrowHeadFlag;
	}

	friend std::ostream& operator << (std::ostream& os,
									  const RS_LeaderData& /*ld*/);

	/** true: leader has an arrow head. false: no arrow. */
	bool arrowHead;
};



/**
 * Class for a leader entity (kind of a polyline arrow).
 *
 * @author Andrew Mustun
 */
class RS_Leader : public RS_EntityContainer {
public:
	RS_Leader(RS_EntityContainer* parent=NULL);
	RS_Leader(RS_EntityContainer* parent,
			  const RS_LeaderData& d);

	RS_Entity* clone() const override;

	/**	@return RS2::EntityDimLeader */
	RS2::EntityType rtti() const override{
		return RS2::EntityDimLeader;
	}

	void update() override;

	/** @return Copy of data that defines the leader. */
	RS_LeaderData getData() const {
		return data;
	}

	/** @return true: if this leader has an arrow at the beginning. */
	bool hasArrowHead() {
		return data.arrowHead;
	}

	RS_Entity* addVertex(const RS_Vector& v);
	void addEntity(RS_Entity* entity) override;

	//	double getLength() const {
	//		return -1.0;
	//	}


	void move(const RS_Vector& offset) override;
	void rotate(const RS_Vector& center, const double& angle) override;
	void rotate(const RS_Vector& center, const RS_Vector& angleVector) override;
	void scale(const RS_Vector& center, const RS_Vector& factor) override;
	void mirror(const RS_Vector& axisPoint1, const RS_Vector& axisPoint2) override;
	void stretch(const RS_Vector& firstCorner,
				 const RS_Vector& secondCorner,
				 const RS_Vector& offset) override;

	friend std::ostream& operator << (std::ostream& os, const RS_Leader& l);

protected:
	RS_LeaderData data;
	bool empty;
};

#endif
