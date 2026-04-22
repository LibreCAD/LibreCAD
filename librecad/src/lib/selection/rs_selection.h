/*
 * ********************************************************************************
 * This file is part of the LibreCAD project, a 2D CAD program
 *
 * Copyright (C) 2025 LibreCAD.org
 * Copyright (C) 2025 sand1024
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 * ********************************************************************************
 */

#ifndef RS_SELECTION_H
#define RS_SELECTION_H
#include <QList>
#include <QSet>

#include "lc_selectionpredicate.h"
#include "rs_document.h"
#include "rs_graphicview.h"

class RS_Document;

namespace RS2
{
    enum EntityType : unsigned;
}

class QString;

class LC_GraphicViewport;
class RS_EntityContainer;
class RS_Graphic;
class RS_Vector;
class RS_Entity;

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
    using FunEntityMatch  = std::function<bool(RS_Entity*)>;

    struct CurrentSelectionState {
        QMap<RS2::EntityType, int> documentEntityTypes;
        QMap<RS2::EntityType, int> selectedEntityTypes;

        int totalDocumentEntities {0};
        int totalSelectedEntities {0};

        bool hasSelection() const {
            return !selectedEntityTypes.empty();
        }
    };


    struct ConditionalSelectionOptions {
        enum ApplyTo {
            Selection,
            Document
        };

        ApplyTo applyArea {Document};
        bool includeIntoSelectionSet {false};
        bool appendToSelectionSet {false};
        FunEntityMatch funEntityMatcher;
        FunEntityMatch funRttiMatcher;
        std::function<void()> funCleanup;
        ConditionalSelectionOptions() = default;
    };

    explicit RS_Selection(RS_Document* container,
                 LC_GraphicViewport* graphicView=nullptr);
    explicit RS_Selection(const RS_GraphicView* graphicView);
    static void unselectAllInDocument(RS_Document* document, LC_GraphicViewport* vp);
    static void selectEntitiesList(RS_Document* document, LC_GraphicViewport* vp, const QList<RS_Entity*>& entities, bool doSelect);
    static void selectEntitiesVector(RS_Document* document, LC_GraphicViewport* vp, const std::vector<RS_Entity*>& entities, bool doSelect);
    static void unselectLayer(RS_Document* document, LC_GraphicViewport* vp, RS_Layer* layer);
    void selectSingle(RS_Entity* e) const;
    void selectEntitiesList(const QList<RS_Entity*> &entities, bool select) const;
    void selectIfMatched(const QList<RS_Entity*> &entities, bool select, FunEntityMatch matchFun) const;
    void selectAll(bool select=true) const;
    void deselectAll() const {selectAll(false);}
    void invertSelection() const;
    void selectWindow(RS2::EntityType typeToSelect, const RS_Vector& v1, const RS_Vector& v2,bool select=true, bool cross=false);
    void selectWindow(const QList<RS2::EntityType> &typesToSelect, const RS_Vector& v1, const RS_Vector& v2,bool select=true, bool cross=false);
    void selectIntersected(RS_Entity* entity, bool select) const;
    void deselectWindow(const RS2::EntityType typeToSelect,const RS_Vector& v1, const RS_Vector& v2) {selectWindow(typeToSelect,v1, v2, false);}
    void selectIntersected(const RS_Vector& v1, const RS_Vector& v2,bool select=true) const;
    void deselectIntersected(const RS_Vector& v1, const RS_Vector& v2) const {selectIntersected(v1, v2, false);}
    void selectContour(RS_Entity* e) const;
    void selectLayer(const RS_Entity* e) const;
    void selectLayer(const RS_Layer* layer, bool select) const;
    void conditionalSelection(const ConditionalSelectionOptions &options) const;
    void countSelectedEntities(QMap<RS2::EntityType, int> &entityTypeMaps) const;
    void collectCurrentSelectionState(CurrentSelectionState& selectionState) const;
    void performBulkSelection(const std::function<void(RS_EntityContainer*, LC_GraphicViewport*, RS_Document*)>& fun) const;
    void justSelect(RS_Entity* rsEntity, const bool on = true) const {m_document->select(rsEntity, on);}
protected:
    RS_Document* m_document = nullptr;
    LC_GraphicViewport* m_viewPort = nullptr;
    bool m_additiveSelection = true;

    void doSelectEntitiesWithTypeInWindow(const RS_EntityContainer* container, RS_Document* doc, RS2::EntityType typeToSelect, const RS_Vector& v1, const RS_Vector& v2, bool select, bool cross);
    void doSelectEntitiesWithTypesInWindow(const RS_EntityContainer* container, RS_Document* doc, const QList<RS2::EntityType>& typesToSelect, const RS_Vector& v1, const RS_Vector& v2, bool select, bool cross);
    void doSelectIntersectedContainer(RS_Entity* entity, RS_Document* doc, bool select) const;
    void doSelectIntersectedAtomic(const RS_Entity* entity, RS_Document* doc, bool select) const;
    void doUnselectAll(const RS_EntityContainer* container, const RS_Document* doc) const;
    void doUnselectAllIfNeeded(const RS_EntityContainer* container, const RS_Document* doc) const;
};

#endif
