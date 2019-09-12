/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2019 Shawn Curry (noneyabiz@mail.wasent.cz)
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
#ifndef LC_SEARCHGRAPH_H
#define LC_SEARCHGRAPH_H
#include "rs_entity.h"
#include "rs_atomicentity.h"
#include "rs_vector.h"

/**
  A* expands paths that are already less expensive by using this function:

  f(n)=g(n)+h(n),

  where

	f(n) = total estimated cost of path through node n
	g(n) = cost so far to reach node n
	h(n) = estimated cost from n to goal. This is the heuristic part of the cost function, so it is like a guess.
*/

/*
	A weighted graph node for A-star/Dijkstra pathfinding in LibreCAD.
	Each node represents a found intersection in our "neighboring" (atomic) entities
	@author Shawn Curry
*/
class LC_SearchNode {
public:
	LC_SearchNode(RS_AtomicEntity* entity, const RS_Vector& intersection, LC_SearchNode* parent = nullptr)
		: parent(parent), entity(entity), intersection(intersection) {
	}

	double calcH(RS_Entity* /*dest*/); // An "admissible" heuristic for LibreCAD may not exist.  
	double calcG();
	double calcF(RS_Entity* dest);

public:
	LC_SearchNode* parent;
	RS_AtomicEntity* entity;
	RS_Vector intersection;
	double f, g, h;
};

/*
	A type of object which can perform A-star/Dijkstra pathfinding for LibreCAD, and plot the resulting path.
	@author Shawn Curry
*/
class LC_SearchGraph {
public:
	LC_SearchGraph() : container(nullptr) {}
	LC_SearchGraph(RS_EntityContainer* container) : container(container) {}
	virtual ~LC_SearchGraph() = default;

	bool findShortestPath(RS_AtomicEntity* start, RS_AtomicEntity* dest, const RS_Vector& cursor, QList<RS_Entity*>* preview);

protected:
	virtual void plotPath(LC_SearchNode* tail, const RS_Vector& cursor, QList<RS_Entity*>* preview) = 0;

private:
	bool contains(QList<LC_SearchNode*>* list, RS_AtomicEntity* e, const RS_Vector& x);
protected:
	RS_EntityContainer* container;
};

/*
	A type of SearchGraph which can construct (or preview) the new path, and perform actions on the resulting entities.
	Entities along the path are exploded and divided as necessary.
	@author Shawn Curry
*/
class LC_SeachGraphPlotter : public LC_SearchGraph
{
public:
	LC_SeachGraphPlotter() : LC_SearchGraph(), graphicView(nullptr) {}
	LC_SeachGraphPlotter(RS_EntityContainer* entityContainer, RS_GraphicView* graphicView)
		: LC_SearchGraph(entityContainer), graphicView(graphicView) {}
	virtual ~LC_SeachGraphPlotter() = default;

	virtual void plotEntity(RS_AtomicEntity* e) = 0;

protected:
	void plotPath(LC_SearchNode* tail, const RS_Vector& cursor, QList<RS_Entity*>* preview) override;

private:
	bool explodePath(LC_SearchNode* tail);

protected:
	RS_GraphicView* graphicView;
};

/*
	An object which can find and select (or preview) the shortest path between 2 Entities using
	the A-star/Dijkstra algorithm.
	@author Shawn Curry
*/
class LC_SelectionSearch : public LC_SeachGraphPlotter {
public:
	LC_SelectionSearch() = delete;
	LC_SelectionSearch(RS_EntityContainer* entityContainer, RS_GraphicView* graphicView) 
		: LC_SeachGraphPlotter(entityContainer, graphicView) {}
	virtual ~LC_SelectionSearch() = default;

	bool selectShortestPath(RS_AtomicEntity* start, RS_AtomicEntity* dest, const RS_Vector& cursor, QList<RS_Entity*>* preview);

protected:
	void plotEntity(RS_AtomicEntity* e);
};

#endif // LC_SEARCHGRAPH_H
