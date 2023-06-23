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

#include <QHash>
#include "rs_pen.h"
#include "rs_vector.h"

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
struct RS_MoveData {
    int number = 0;
    bool useCurrentAttributes = false;
    bool useCurrentLayer = false;
    RS_Vector offset;
};


/**
 * Holds the data needed for offset modifications.
 */
struct RS_OffsetData {
    int number = 0;
    bool useCurrentAttributes = false;
    bool useCurrentLayer = false;
    RS_Vector coord;
    double distance = 0.;
};

/**
 * Holds the data needed for rotation modifications.
 */
struct RS_RotateData {
    int number = 0;
    bool useCurrentAttributes = false;
    bool useCurrentLayer = false;
    RS_Vector center;
    double angle = 0.;
};



/**
 * Holds the data needed for scale modifications.
 */
struct RS_ScaleData {
    int number = 0;
    bool useCurrentAttributes = false;
    bool useCurrentLayer = false;
    RS_Vector referencePoint;
    RS_Vector factor;
};


/**
 * Holds the data needed for mirror modifications.
 */
struct RS_MirrorData {
    bool copy = false;
    bool useCurrentAttributes = false;
    bool useCurrentLayer = false;
    RS_Vector axisPoint1;
    RS_Vector axisPoint2;
};


/**
 * Holds the data needed for move/rotate modifications.
 */
struct RS_MoveRotateData {
    int number = 0;
    bool useCurrentAttributes = false;
    bool useCurrentLayer = false;
    RS_Vector referencePoint;
    RS_Vector offset;
    double angle = 0.;
};



/**
 * Holds the data needed for rotation around two centers modifications.
 */
struct RS_Rotate2Data {
    int number = 0;
    bool useCurrentAttributes = false;
    bool useCurrentLayer = false;
    RS_Vector center1;
    RS_Vector center2;
    double angle1 = 0.;
    double angle2 = 0.;
};



/**
 * Holds the data needed for beveling modifications.
 */
struct RS_BevelData
{
    double length1 = 0.;
    double length2 = 0.;
    bool trim = false;
};




/**
 * Holds the data needed for rounding modifications.
 */
struct RS_RoundData {
    double radius = 0.;
    bool trim = false;
};


/**
 * Holds the data needed for moving reference points.
 */
struct RS_MoveRefData {
    RS_Vector ref;
    RS_Vector offset;
};



/**
 * Holds the data needed for changing attributes.
 */
struct RS_AttributesData {
        QString layer;
        RS_Pen pen;
        bool changeLayer = false;
        bool changeColor = false;
        bool changeLineType = false;
        bool changeWidth = false;
        bool applyBlockDeep = false;
};


/**
 * Holds the data needed for pasting.
 */
struct RS_PasteData {
        RS_PasteData(RS_Vector insertionPoint,
                double factor,
                double angle,
                bool asInsert,
				const QString& blockName);

        //! Insertion point.
        RS_Vector insertionPoint;
        //! Scale factor.
        double factor = 1.;
        //! Rotation angle.
        double angle = 0.;
        //! Paste as an insert rather than individual entities.
        bool asInsert = false;
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
    RS_Modification(RS_EntityContainer& entityContainer,
                    RS_GraphicView* graphicView=nullptr,
                                        bool handleUndo=true);

	void remove();
	void revertDirection();
	bool changeAttributes(RS_AttributesData& data);
    bool changeAttributes(RS_AttributesData& data, RS_EntityContainer* container);

    void copy(const RS_Vector& ref, const bool cut);

public:
    void paste(const RS_PasteData& data, RS_Graphic* source=nullptr);

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
    void copyEntity(RS_Entity* e, const RS_Vector& ref, bool cut);
    void copyLayers(RS_Entity* e);
    void copyBlocks(RS_Entity* e);
    bool pasteLayers(RS_Graphic* source);
    bool pasteContainer(RS_Entity* entity, RS_EntityContainer* container, QHash<QString, QString>blocksDict, RS_Vector insertionPoint);
    bool pasteEntity(RS_Entity* entity, RS_EntityContainer* container);
    void deselectOriginals(bool remove);
	void addNewEntities(std::vector<RS_Entity*>& addList);
	bool explodeTextIntoLetters(RS_MText* text, std::vector<RS_Entity*>& addList);
	bool explodeTextIntoLetters(RS_Text* text, std::vector<RS_Entity*>& addList);

protected:
    RS_EntityContainer* container =nullptr;
    RS_Graphic* graphic = nullptr;
    RS_Document* document = nullptr;
    RS_GraphicView* graphicView = nullptr;
    bool handleUndo = false;
};

#endif
