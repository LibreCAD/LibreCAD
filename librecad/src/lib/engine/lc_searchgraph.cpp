/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2019 Shawn Curry (noneyabiz@mail.wasent.cz)

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
#include "lc_searchgraph.h"
#include "lc_undosection.h"
#include "rs_information.h"
#include "rs_modification.h"
#include "rs_entitycontainer.h"
#include "rs_graphicview.h"
#include "rs_dialogfactory.h"
#include "rs_arc.h"
#include "rs_circle.h"

/**
  A* expands paths that are already less expensive by using this function:

  f(n)=g(n)+h(n),

  where

	f(n) = total estimated cost of path through node n
	g(n) = cost so far to reach node n
	h(n) = estimated cost from n to goal. This is the heuristic part of the cost function, so it is like a guess.
*/
inline double LC_SearchNode::calcH(RS_Entity *) {
	// TODO: simple distance is not an "admissible" heuristic for us.
	return 0.0;
}

inline double LC_SearchNode::calcG() {
	g = 0;
	if (parent) {
		if (parent->intersection.valid)
			g = parent->entity->getLengthBetween(parent->intersection, intersection);
		else
			g = fmax(parent->entity->getLengthBetween(intersection, parent->entity->getStartpoint()),
				parent->entity->getLengthBetween(intersection, parent->entity->getEndpoint()));
		g += parent->g;
	}
	return g;
}

inline double LC_SearchNode::calcF(RS_Entity * dest) {
	f = calcG() + calcH(dest);
	return f;
}

/**
  The A* algorithm begins at the starting cell, and considers all adjacent cells.  It then picks the cell with the lowest cost,
  which is the estimated f(n).  This process is recursively repeated until the shortest path has been found to the target. The computation
  of f(n) is done via a heuristic that usually gives good results. The heuristic function must be admissible, which means it can never
  overestimate the cost to reach the goal. Both the Manhattan distance and h(n) = 0 are admissible.
*/
bool LC_SearchGraph::findShortestPath(RS_AtomicEntity * start, RS_AtomicEntity * dest, const RS_Vector & cursor, QList<RS_Entity*>* preview)
{
	if (!dest || !start)
		return false;

	const int MAX_PATHS = 100;
	QList<LC_SearchNode*> open, closed, created;
	LC_SearchNode* root = new LC_SearchNode(start, RS_Vector(false));
	root->calcF(dest);
	open.push_back(root);
	while (open.size() > 0)
	{
		if (open.size() > MAX_PATHS) { // this is either a very complicated drawing, or there is a bug
			for (LC_SearchNode* sn : created)
				delete sn;
			return false; // give up
		}

		LC_SearchNode* current = open.takeFirst();

		if (current->entity == dest) {
			plotPath(current, cursor, preview);
			for (LC_SearchNode* sn : created)
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

						LC_SearchNode* n = new LC_SearchNode(ae, v, current);
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

		std::sort(open.begin(), open.end(), [](const LC_SearchNode *s1, const LC_SearchNode *s2)-> bool {
			return s1->f < s2->f;
		});
	}

	return false;
}

bool LC_SearchGraph::contains(QList<LC_SearchNode*>* list, RS_AtomicEntity * e, const RS_Vector & x)
{
	for (auto sn : *list) {
		if (!sn->intersection.valid && sn->entity == e)
			return true; // reached the start entity
		else if (sn->entity == e && sn->intersection.valid && sn->intersection.distanceTo(x) < RS_TOLERANCE_TRIM)
			return true;
	}		
	return false;
}

void LC_SeachGraphPlotter::plotPath(LC_SearchNode * tail, const RS_Vector & cursor, QList<RS_Entity*>* preview)
{
	if (!preview)
		explodePath(tail); // this needs it's own undo cycle

	LC_UndoSection undo(container->getDocument());
	RS_Modification modify(*container, graphicView, true);
	QList<RS_Entity*> selected;
	LC_SearchNode *current = tail;
	RS_Vector prevInt = cursor;

	while (current) {
		if (preview) {
			RS_AtomicEntity* seg = static_cast<RS_AtomicEntity*>(current->entity->clone());
			if (!current->intersection.valid) { // this is the starting segment
				double d1 = seg->getLengthBetween(prevInt, seg->getStartpoint());
				double d2 = seg->getLengthBetween(prevInt, seg->getEndpoint());
				if (d1 < d2)
					current->intersection = seg->getStartpoint();
				else
					current->intersection = seg->getEndpoint();
			}
			seg->trimEndpoints(prevInt, current->intersection);
			preview->push_back(seg);
		}
		else {
			QList<RS_AtomicEntity*> pieces;
			if (prevInt == cursor) { // this is the destination entity
				modify.cut(current->intersection, current->entity, &pieces);
				double min = RS_MAXDOUBLE;
				for (auto p : pieces) {
					double d = p->getDistanceToPoint(cursor);
					if (d < min) {
						min = d;
						current->entity = p; // find the segment closest to the cursor
					}
				}
			}
			else {
				if (!current->intersection.valid) { // this is the starting segment
					modify.cut(prevInt, current->entity, &pieces);
					double min = RS_MAXDOUBLE;
					for (auto p : pieces) {
						double l = p->getLength();
						if (l < min) {
							min = l;
							current->entity = p; // find shortest segment
						}
					}
				}
				else { // there are 2 valid intersections; trim to both
					modify.cut2(prevInt, current->intersection, current->entity, &pieces);
					if (current->entity->isClosedContour()) {
						RS_AtomicEntity *test = static_cast<RS_AtomicEntity*>(current->entity->clone());
						test->trimEndpoints(prevInt, current->intersection);
						double length = test->getLength();
						delete test;

						for (auto p : pieces) {
							if (fabs(length - p->getLength()) < RS_TOLERANCE) {
								current->entity = p;
								break;
							}
						}
					}
					else
					{
						for (auto p : pieces) {
							if (p->hasEndpoints(prevInt, current->intersection, RS_TOLERANCE_TRIM)) {
								current->entity = p; // this is our segment
								break;
							}
						}
					}
				}
			}
			// the current entity has been trimmed and the new entity located
			plotEntity(current->entity);
		}
		prevInt = current->intersection;
		current = current->parent;
	}
}

bool LC_SeachGraphPlotter::explodePath(LC_SearchNode * tail)
{
	int result = 0;
	LC_UndoSection undo(container->getDocument());
	LC_SearchNode *current = tail;
	while (current) {
		if (current->entity->getParent() != container) {
			RS_Entity *parent = current->entity;
			RS_Entity *ancestor = current->entity->getParent();
			while (ancestor && ancestor != container) {
				parent = ancestor;
				ancestor = parent->getParent();
			}
			if (ancestor) {
				parent->setSelected(true);
				result++;
			}
		}
		current = current->parent;
	}
	if (result) {
		RS_Modification modify(*container, graphicView, true);
		modify.explode();
		current = tail;
		while (current) {
			if (!current->entity->isVisible()) {
				bool found = false;
				RS_Entity* e = container->firstEntity();
				while (!found && e) {
					if (e->getStartpoint() == current->entity->getStartpoint()
						&& e->getEndpoint() == current->entity->getEndpoint()) {
						current->entity = static_cast<RS_AtomicEntity*>(e);
						found = true;
					}
					e = container->nextEntity();
				}
			}
			current = current->parent;
		}
	}
	current = tail;
	while (current) {
		if (current->entity->rtti() == RS2::EntityCircle) { // also "explode" circles into arcs
			RS_Circle* c = static_cast<RS_Circle*>(current->entity);
			RS_ArcData d(c->getCenter(), c->getRadius(), 0, 2.*M_PI, false);
			current->entity->setUndoState(true);
			undo.addUndoable(current->entity);
			current->entity = new RS_Arc(current->entity->getParent(), d);
			container->addEntity(current->entity);
			undo.addUndoable(current->entity);
			result++;
		}
		current = current->parent;
	}
	if (result == 1)
		RS_DIALOGFACTORY->commandMessage(QString(QObject::tr("1 entity exploded")));
	else if (result > 0)
		RS_DIALOGFACTORY->commandMessage(QString(QObject::tr("%1 entities exploded")).arg(result));

	return result > 0;
}

bool LC_SelectionSearch::selectShortestPath(RS_AtomicEntity * start, RS_AtomicEntity * dest, const RS_Vector & cursor, QList<RS_Entity*>* preview)
{
	if (!container)
		return false;
	return findShortestPath(start, dest, cursor, preview);
}

void LC_SelectionSearch::plotEntity(RS_AtomicEntity * e)
{
	if (graphicView) {
		graphicView->deleteEntity(e);
	}

	e->setSelected(true);

	if (graphicView) {
		graphicView->drawEntity(e);
	}
}
