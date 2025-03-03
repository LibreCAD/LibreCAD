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
#include "rs_arc.h"

class RS_AtomicEntity;
class RS_Entity;
class RS_EntityContainer;
class RS_MText;
class RS_Text;
class RS_Line;
class RS_Polyline;
class RS_Document;
class RS_Graphic;
class RS_GraphicView;

struct LC_ModifyOperationFlags{
    bool useCurrentAttributes = false;
    bool useCurrentLayer = false;
    bool keepOriginals = false;
    int number = 0;
    bool multipleCopies = false;

    int obtainNumberOfCopies() const {
        int numberOfCopies = number;
        if (!multipleCopies){
            numberOfCopies = 1;
        }
        else{
            if (numberOfCopies < 1){
                numberOfCopies = 1;
            }
        }
        return numberOfCopies;
    }
};

/**
 * Holds the data needed for move modifications.
 */
struct RS_MoveData : public LC_ModifyOperationFlags{
    RS_Vector offset;
};

struct RS_BoundData{
    RS_BoundData(const RS_Vector &left, const RS_Vector &top) {
        min = left;
        max = top;
    }

    RS_Vector min;
    RS_Vector max;

    RS_Vector getCenter(){
        return (min+max)/2;
    }
};


/**
 * Holds the data needed for offset modifications.
 */
struct RS_OffsetData : public LC_ModifyOperationFlags{
    RS_Vector coord;
    double distance = 0.;
};

/**
 * Holds the data needed for rotation modifications.
 */
struct RS_RotateData : public LC_ModifyOperationFlags{
    RS_Vector center;
    RS_Vector refPoint;
    double angle = 0.;
    double secondAngle = 0.0;
    bool secondAngleIsAbsolute = false;
    bool twoRotations = false;
};

/**
 * Holds the data needed for scale modifications.
 */
struct RS_ScaleData : public LC_ModifyOperationFlags {
    RS_Vector referencePoint;
    RS_Vector factor = RS_Vector(1.1,1.0,0.0);
    // Find the factor by a source and a target point
    bool isotropicScaling = true;
    bool toFindFactor = false;
};


/**
 * Holds the data needed for mirror modifications.
 */
struct RS_MirrorData : public LC_ModifyOperationFlags{
    RS_Vector axisPoint1;
    RS_Vector axisPoint2;
};


/**
 * Holds the data needed for move/rotate modifications.
 */
struct RS_MoveRotateData : public LC_ModifyOperationFlags{
    RS_Vector referencePoint;
    RS_Vector offset;
    double angle = 0.;
    bool sameAngleForCopies = false;
};


/**
 * Holds the data needed for rotation around two centers modifications.
 */
struct RS_Rotate2Data : public LC_ModifyOperationFlags{
    RS_Vector center1;
    RS_Vector center2;

    double angle1 = 0.;
    double angle2 = 0.;
    bool mirrorAngles = false;
    bool sameAngle2ForCopies = false;
};


/**
 * Holds the data needed for beveling modifications.
 */
struct RS_BevelData{
    double length1 = 0.;
    double length2 = 0.;
    bool trim = false;
};

struct LC_TrimResult{
    bool result = false;
    RS_Entity* trimmed1 = nullptr;
    RS_Entity* trimmed2 = nullptr;
    RS_Vector intersection1;
    RS_Vector intersection2;
};

struct LC_BevelResult{
    RS_Line* bevel = nullptr;
    bool polyline = false;
    RS_Entity* trimmed1 = nullptr;
    RS_Entity* trimmed2 = nullptr;
    bool trimStart1 = false;
    bool trimStart2 = false;
    RS_Vector intersectionPoint = RS_Vector(false);
    int error = OK;

    enum{
        OK,
        ERR_NO_INTERSECTION,
        ERR_NOT_THE_SAME_POLYLINE
    };
};

struct LC_RoundResult{
    RS_Arc* round = nullptr;
    bool polyline = false;
    RS_Entity* trimmed1 = nullptr;
    RS_Entity* trimmed2 = nullptr;
    int trim1Mode = false;
    int trim2Mode = false;
    RS_Vector trimmingPoint1 = RS_Vector(false);
    RS_Vector trimmingPoint2 = RS_Vector(false);
    RS_Vector intersectionPoint = RS_Vector(false);
    int error = OK;

    enum {
        TRIM_START,
        TRIM_END,
        TRIM_CIRCLE
    };

    enum{
        OK,
        ERR_NO_INTERSECTION,
        ERR_NOT_THE_SAME_POLYLINE,
        NO_PARALLELS
    };
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
    RS_Modification(
        RS_EntityContainer &entityContainer,
        RS_GraphicView *graphicView = nullptr,
        bool handleUndo = true);
    void remove();
    void remove(const std::vector<RS_Entity*> &entitiesList);
    void revertDirection();
    void revertDirection(const std::vector<RS_Entity*> &entitiesList);
    bool changeAttributes(RS_AttributesData &data);
    bool changeAttributes(RS_AttributesData& data, const std::vector<RS_Entity*> &entitiesList,RS_EntityContainer *container);
    bool changeAttributes(RS_AttributesData &data, RS_EntityContainer *container);
    void copy(const RS_Vector &ref, const bool cut);
    void paste(const RS_PasteData &data, RS_Graphic *source = nullptr);
    bool move(RS_MoveData &data, bool previewOnly = false, RS_EntityContainer* previewContainer = nullptr);
    bool move(RS_MoveData& data, const std::vector<RS_Entity*> &entitiesList, bool forPreviewOnly, bool keepSelected);
    bool rotate(RS_RotateData &data, bool forPreviewOnly = false);
    bool rotate(RS_RotateData &data, const std::vector<RS_Entity *> &entitiesList, bool forPreviewOnly);
    bool scale(RS_ScaleData &data, bool forPreviewOnly = false);
    bool scale(RS_ScaleData &data, const std::vector<RS_Entity *> &entitiesList, bool forPreviewOnly);
    bool mirror(RS_MirrorData &data);
    bool mirror(RS_MirrorData &data, const std::vector<RS_Entity*> &entitiesList, bool forPreviewOnly);
    bool moveRotate(RS_MoveRotateData &data, bool previewOnly = false, RS_EntityContainer* previewContainer = nullptr);
    bool moveRotate(RS_MoveRotateData &data, const std::vector<RS_Entity*> &entitiesList, bool previewOnly = false);
    bool rotate2(RS_Rotate2Data &data, bool forPreviewOnly);
    bool rotate2(RS_Rotate2Data& data, const std::vector<RS_Entity*> &entitiesList, bool forPreviewOnly);
    LC_TrimResult trim(
        const RS_Vector &trimCoord, RS_AtomicEntity *trimEntity,
        const RS_Vector &limitCoord, RS_Entity *limitEntity,
        bool both, bool forPreview = false);
    RS_Entity* trimAmount(const RS_Vector &trimCoord, RS_AtomicEntity *trimEntity,
                          double dist, bool trimBoth, bool &trimStart, bool &trimEnd, bool forPreview = false);
    bool offset(const RS_OffsetData &data, bool previewOny = false, RS_EntityContainer* previewContainer = nullptr);
    bool offset(const RS_OffsetData& data, const std::vector<RS_Entity*> &entitiesList, bool forPreviewOnly, bool keepSelected);
    bool cut(const RS_Vector &cutCoord, RS_AtomicEntity *cutEntity);
    bool stretch(
        const RS_Vector &firstCorner,
        const RS_Vector &secondCorner,
        const RS_Vector &offset,
        bool removeOriginals);
    LC_BevelResult* bevel(
        const RS_Vector &coord1, RS_AtomicEntity *entity1,
        const RS_Vector &coord2, RS_AtomicEntity *entity2,
        RS_BevelData &data, bool previewOnly);
    LC_RoundResult* round(
        const RS_Vector &coord,
        const RS_Vector &coord1,
        RS_AtomicEntity *entity1,
        const RS_Vector &coord2,
        RS_AtomicEntity *entity2,
        RS_RoundData &data);
    bool explode(const bool remove = true, const bool forceUndoable = false);
    bool explode(const std::vector<RS_Entity*> &entitiesList, const bool remove = true, const bool forceUndoableOperation = false);
    bool explodeTextIntoLetters();
    bool explodeTextIntoLetters(const std::vector<RS_Entity*> &entitiesList);
    bool moveRef(RS_MoveRefData &data);
    void deleteLineNode(RS_Line *polyline, const RS_Vector &node);
    bool splitPolyline(
        RS_Polyline &polyline,
        RS_Entity &e1, RS_Vector v1,
        RS_Entity &e2, RS_Vector v2,
        RS_Polyline **polyline1,
        RS_Polyline **polyline2) const;
    RS_Polyline *addPolylineNode(
        RS_Polyline &polyline,
        const RS_AtomicEntity &segment,
        const RS_Vector &node);
    RS_Polyline *deletePolylineNode(
        RS_Polyline &polyline,
        const RS_Vector &node, bool createOnly);
    RS_Polyline *deletePolylineNodesBetween(
        RS_Polyline &polyline,
        const RS_Vector &node1, const RS_Vector &node2, bool createOnly);
    RS_Polyline *polylineTrim(
        RS_Polyline &polyline,
        RS_AtomicEntity &segment1,
        RS_AtomicEntity &segment2,
        bool createOnly);
    static RS_BoundData getBoundingRect(std::vector<RS_Entity *> &selected); // todo - probably it should be located in other utility class..
    void collectSelectedEntities(std::vector<RS_Entity *> &entitiesList) const;
private:
    void copyEntity(RS_Entity *e, const RS_Vector &ref, bool cut);
    void copyLayers(RS_Entity *e);
    void copyBlocks(RS_Entity *e);
    bool pasteLayers(RS_Graphic *source);
    bool pasteContainer(RS_Entity *entity, RS_EntityContainer *containerToPaste, QHash<QString, QString> blocksDict, RS_Vector insertionPoint);
    bool pasteEntity(RS_Entity *entity, RS_EntityContainer *containerToPaste);
    void deselectOriginals(bool remove);
    void deselectOriginals(const std::vector<RS_Entity*> &entitiesList, bool remove);
    void addNewEntities(const std::vector<RS_Entity *> &addList, bool forceUndoable = false);
    bool explodeTextIntoLetters(RS_MText *text, std::vector<RS_Entity *> &addList);
    bool explodeTextIntoLetters(RS_Text *text, std::vector<RS_Entity *> &addList);

protected:
    RS_EntityContainer *container = nullptr;
    RS_Graphic *graphic = nullptr;
    RS_Document *document = nullptr;
    RS_GraphicView *graphicView = nullptr;
    bool handleUndo = false;

    void trimEnding(const RS_Vector &trimCoord, RS_AtomicEntity *trimmed1, const RS_Vector &is) const;

    void deleteOriginalAndAddNewEntities(const std::vector<RS_Entity *> &addList, const std::vector<RS_Entity*> &originalEntities,
                                         bool addOnly, bool deleteOriginals, bool forceUndoable = false);
};

#endif
