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

#ifndef RS_MODIFICATION_H
#define RS_MODIFICATION_H

#include "rs_vector.h"
#include "rs_pen.h"
#include <QHash>

class RS_AtomicEntity;
class RS_Entity;
class RS_EntityContainer;
class RS_MText;
class RS_Text;
class RS_Polyline;
class RS_Document;
class RS_Graphic;
class RS_GraphicView;

/**
 * Holds the data needed for move modifications.
 */
class RS_MoveData {
public:
    int number;
    bool useCurrentAttributes;
    bool useCurrentLayer;
    RS_Vector offset;
};


/**
 * Holds the data needed for offset modifications.
 */
class RS_OffsetData {
public:
    int number;
    bool useCurrentAttributes;
    bool useCurrentLayer;
    RS_Vector coord;
    double distance;
};

/**
 * Holds the data needed for rotation modifications.
 */
class RS_RotateData {
public:
    int number;
    bool useCurrentAttributes;
    bool useCurrentLayer;
    RS_Vector center;
    double angle;
};



/**
 * Holds the data needed for scale modifications.
 */
class RS_ScaleData {
public:
    int number;
    bool useCurrentAttributes;
    bool useCurrentLayer;
    RS_Vector referencePoint;
    RS_Vector factor;
};


/**
 * Holds the data needed for mirror modifications.
 */
class RS_MirrorData {
public:
    bool copy;
    bool useCurrentAttributes;
    bool useCurrentLayer;
    RS_Vector axisPoint1;
    RS_Vector axisPoint2;
};


/**
 * Holds the data needed for move/rotate modifications.
 */
class RS_MoveRotateData {
public:
    int number;
    bool useCurrentAttributes;
    bool useCurrentLayer;
    RS_Vector referencePoint;
        RS_Vector offset;
        double angle;
};



/**
 * Holds the data needed for rotation around two centers modifications.
 */
class RS_Rotate2Data {
public:
    int number;
    bool useCurrentAttributes;
    bool useCurrentLayer;
    RS_Vector center1;
    RS_Vector center2;
    double angle1;
    double angle2;
};



/**
 * Holds the data needed for beveling modifications.
 */
class RS_BevelData {
public:
        double length1;
        double length2;
        bool trim;
};




/**
 * Holds the data needed for rounding modifications.
 */
class RS_RoundData {
public:
        double radius;
        bool trim;
};


/**
 * Holds the data needed for moving reference points.
 */
class RS_MoveRefData {
public:
        RS_Vector ref;
    RS_Vector offset;
};



/**
 * Holds the data needed for changing attributes.
 */
class RS_AttributesData {
public:
        QString layer;
        RS_Pen pen;
        bool changeLayer;
        bool changeColor;
        bool changeLineType;
        bool changeWidth;
};


/**
 * Holds the data needed for pasting.
 */
class RS_PasteData {
public:
        RS_PasteData(RS_Vector insertionPoint,
                double factor,
                double angle,
                bool asInsert,
				const QString& blockName);

        //! Insertion point.
        RS_Vector insertionPoint;
        //! Scale factor.
        double factor;
        //! Rotation angle.
        double angle;
        //! Paste as an insert rather than individual entities.
        bool asInsert;
        //! Name of the block to create or an empty string to assign a new auto name.
        QString blockName;
};


/**
 * API Class for manipulating entities.
 * There's no interaction handled in this class.
 *
 * All modifications can be undone / redone if the container
 * is a RS_Graphic.
 *
 * This class is connected to an entity container and
 * can be connected to a graphic view.
 *
 * @author Andrew Mustun
 */
class RS_Modification {
public:
	RS_Modification()=delete;
    RS_Modification(RS_EntityContainer& entityContainer,
                    RS_GraphicView* graphicView=NULL,
                                        bool handleUndo=true);

	void remove();
	void revertDirection();
	bool changeAttributes(RS_AttributesData& data);
    bool changeAttributes(RS_AttributesData& data, RS_EntityContainer* container, std::vector<RS_Entity*> addList);

        void copy(const RS_Vector& ref, const bool cut);
private:
        void copyEntity(RS_Entity* e, const RS_Vector& ref, const bool cut);
        void copyLayers(RS_Entity* e);
        void copyBlocks(RS_Entity* e);
        bool pasteLayers(RS_Graphic* source);
        bool pasteContainer(RS_Entity* entity, RS_EntityContainer* container, QHash<QString, QString>blocksDict, RS_Vector insertionPoint);
        bool pasteEntity(RS_Entity* entity, RS_EntityContainer* container);
public:
        void paste(const RS_PasteData& data, RS_Graphic* source=NULL);

    bool move(RS_MoveData& data);
    bool rotate(RS_RotateData& data);
    bool scale(RS_ScaleData& data);
    bool mirror(RS_MirrorData& data);
    bool moveRotate(RS_MoveRotateData& data);
    bool rotate2(RS_Rotate2Data& data);

    bool trim(const RS_Vector& trimCoord, RS_AtomicEntity* trimEntity,
              const RS_Vector& limitCoord, RS_Entity* limitEntity,
              bool both);
    bool trimAmount(const RS_Vector& trimCoord, RS_AtomicEntity* trimEntity,
                    double dist);
    bool offset(const RS_OffsetData& data);
    bool cut(const RS_Vector& cutCoord, RS_AtomicEntity* cutEntity);
    bool stretch(const RS_Vector& firstCorner,
                                const RS_Vector& secondCorner,
                                const RS_Vector& offset);

    bool bevel(const RS_Vector& coord1, RS_AtomicEntity* entity1,
              const RS_Vector& coord2, RS_AtomicEntity* entity2,
                          RS_BevelData& data);
    bool round(const RS_Vector& coord,
               const RS_Vector& coord1,
                   RS_AtomicEntity* entity1,
               const RS_Vector& coord2,
               RS_AtomicEntity* entity2,
                           RS_RoundData& data);

        bool explode(const bool remove = true);
		bool explodeTextIntoLetters();
        bool moveRef(RS_MoveRefData& data);

    bool splitPolyline(RS_Polyline& polyline,
                       RS_Entity& e1, RS_Vector v1,
                       RS_Entity& e2, RS_Vector v2,
                       RS_Polyline** polyline1,
                       RS_Polyline** polyline2) const;
        RS_Polyline* addPolylineNode(RS_Polyline& polyline,
                     const RS_AtomicEntity& segment,
                                 const RS_Vector& node);
        RS_Polyline* deletePolylineNode(RS_Polyline& polyline,
                                const RS_Vector& node);
        RS_Polyline* deletePolylineNodesBetween(RS_Polyline& polyline, RS_AtomicEntity& segment,
                                const RS_Vector& node1, const RS_Vector& node2);
        RS_Polyline* polylineTrim(RS_Polyline& polyline,
                                RS_AtomicEntity& segment1,
                                RS_AtomicEntity& segment2);

private:
    void deselectOriginals(bool remove);
	void addNewEntities(std::vector<RS_Entity*>& addList);
	bool explodeTextIntoLetters(RS_MText* text, std::vector<RS_Entity*>& addList);
	bool explodeTextIntoLetters(RS_Text* text, std::vector<RS_Entity*>& addList);

protected:
    RS_EntityContainer* container;
    RS_Graphic* graphic;
    RS_Document* document;
    RS_GraphicView* graphicView;
        bool handleUndo;
};

#endif
