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

#ifndef RS_SELECTION_H
#define RS_SELECTION_H

#include "rs_entitycontainer.h"
#include "rs_graphicview.h"
#include "rs_information.h"


/**
 * API Class for selecting entities. 
 * There's no interaction handled in this class.
 * This class is connected to an entity container and
 * can be connected to a graphic view.
 *
 * @author Andrew Mustun
 */
class RS_Selection {
public:
    RS_Selection(RS_EntityContainer& entityContainer,
                 RS_GraphicView* graphicView=NULL);

    void selectSingle(RS_Entity* e);
    void selectAll(bool select=true);
    void deselectAll() {
        selectAll(false);
    }
    void invertSelection();
    void selectWindow(const RS_Vector& v1, const RS_Vector& v2,
                      bool select=true, bool cross=false);
    void deselectWindow(const RS_Vector& v1, const RS_Vector& v2) {
        selectWindow(v1, v2, false);
    }
    void selectIntersected(const RS_Vector& v1, const RS_Vector& v2,
                      bool select=true);
    void deselectIntersected(const RS_Vector& v1, const RS_Vector& v2) {
		selectIntersected(v1, v2, false);
	}
    void selectContour(RS_Entity* e, QList<RS_Entity*>* preview = nullptr);
	void selectContour(RS_Entity* start, RS_Entity* end, const RS_Vector& cursor, QList<RS_Entity*>* preview = nullptr);
	
    void selectLayer(RS_Entity* e);
    void selectLayer(const QString& layerName, bool select=true);
    void deselectLayer(QString& layerName) {
		selectLayer(layerName, false);
	}

private:	
	void setSelected(QList<RS_Entity*>& list, bool select = true);
	bool findContour(RS_Entity* start, RS2::Ending end, QList<RS_Entity*>& list);
	bool findContour_aStar(RS_Entity* start, RS_Entity* dest, RS2::Ending end, QList<RS_Entity*>& list);

protected:
    RS_EntityContainer* container;
    RS_Graphic* graphic;
    RS_GraphicView* graphicView;
};

/*
  A* expands paths that are already less expensive by using this function:

  f(n)=g(n)+h(n),

  where

	f(n) = total estimated cost of path through node n
	g(n) = cost so far to reach node n
	h(n) = estimated cost from n to goal. This is the heuristic part of the cost function, so it is like a guess.

In the grid above, A* algorithm begins at the start (red node), and considers all adjacent cells. Once the list of adjacent cells has been populated, 
it filters out those which are inaccessible (walls, obstacles, out of bounds). It then picks the cell with the lowest cost, which is the estimated f(n). 
This process is recursively repeated until the shortest path has been found to the target (blue node). The computation of f(n) is done via a heuristic 
that usually gives good results.

The calculation of h(n) can be done in various ways:

The Manhattan distance (explained below) from node n to the goal is often used. This is a standard heuristic for a grid.

If h(n) = 0, A* becomes Dijkstra's algorithm, which is guaranteed to find a shortest path.

The heuristic function must be admissible, which means it can never overestimate the cost to reach the goal. Both the Manhattan distance and h(n) = 0 are admissible.
*/
#include "rs_atomicentity.h"

class SearchNode {
public:
	SearchNode(RS_AtomicEntity* entity, const RS_Vector& intersection, SearchNode* parent = nullptr)
		: parent(parent), entity(entity), intersection(intersection) {
	}

	double calcH(RS_Entity* dest) {
		h = RS_MAXDOUBLE;
		if (dest)
			h = dest->getDistanceToPoint(intersection);		
		return h;
	}

	double calcG() {
		g = 0;
		if (parent) {
			if (parent->intersection.valid)
				g = parent->g + parent->entity->getLengthBetween(parent->intersection, intersection);
			else
				g = parent->g + fmax(parent->entity->getLengthBetween(intersection, parent->entity->getStartpoint()), 
					parent->entity->getLengthBetween(intersection, parent->entity->getEndpoint()));
		}			
		return g;
	}

	double calcF(RS_Entity* dest) {
		f = calcG() + calcH(dest);
		return f;
	}

	bool contains(QList<SearchNode*>* list, RS_AtomicEntity* e, const RS_Vector& x) {
		for (auto sn : *list)
			if (sn->entity == e && sn->intersection == x)
				return true;
		return false;
	}

	void plotPath(SearchNode* tail, QList<RS_Entity*>& path) {
		SearchNode *current = tail, *prev = nullptr;
		while (current) {
			RS_AtomicEntity* seg = static_cast<RS_AtomicEntity*>(current->entity->clone());
			/*switch (trimmed1->getTrimPoint(trimCoord, v)) {
			case RS2::EndingStart:
				
				break;
			case RS2::EndingEnd:
				
				break;
			default:
				break;
			}*/
			prev = current;
			current = current->parent;
		}
	}

	bool findPath(RS_EntityContainer* container, RS_AtomicEntity* dest, QList<RS_Entity*>& path)
	{
		if (!container || !dest || !entity)
			return false;

		QList<SearchNode*> open, closed, created;
		this->calcF(dest);
		open.push_back(this);

		while (open.size() > 0)
		{
			SearchNode* current = open.takeFirst();

			if (current->entity == dest) {
				plotPath(current, path);
				for (SearchNode* sn : created)
					delete sn;
				return true;
			}

			closed.push_back(current);

			RS_Entity* e = container->firstEntity(RS2::ResolveAll);
			while (e) {
				if (e != current->entity && e->isAtomic() && e->isVisible() && !e->isLocked() && !e->isConstruction()) {
					RS_AtomicEntity* ae = static_cast<RS_AtomicEntity*>(e);
					RS_VectorSolutions sol = RS_Information::getIntersection(current->entity, ae, true);
					if (sol.hasValid()) {
						for (auto v : sol) {						
							if (contains(&closed, ae, v))
								continue;
							
							SearchNode* n = new SearchNode(ae, v, current);
							if (current->g < n->calcG()) {
								created.push_back(n);
								n->f = n->g + n->calcH(dest);
								if (!contains(&open, ae, v))
									open.push_back(n);
							}
							else delete n;
						}
					}
				}
				e = container->nextEntity(RS2::ResolveAll);
			}

			std::sort(open.begin(), open.end(), [](const SearchNode *s1, const SearchNode *s2)-> bool {
				return s1->f < s2->f;
			});
		}

		return false;
	}
private:
	SearchNode* parent;
	RS_AtomicEntity* entity;
	RS_Vector intersection;
	double f, g, h;
};

#endif
