/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2026 LibreCAD.org
** Copyright (C) 2026 Dongxu Li
**
** This program is free software; you can redistribute it and/or
** modify it under the terms of the GNU General Public License
** as published by the Free Software Foundation; either version 2
** of the License, or (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
**********************************************************************/

// The Properties dock shows the current layer as "Visible layer" and "Locked". "Visible layer" was
// checked for a hidden layer, the opposite of the open eye the Layer List and the Layer Tree show.
// A hidden layer can be current (#3229), so the sheet shows it often.

#include <catch2/catch_test_macros.hpp>

#include "lc_actiontestsupport.h"
#include "lc_propertiesprovider_graphic_layer.h"
#include "lc_property_bool.h"
#include "lc_property_container.h"
#include "rs_graphic.h"
#include "rs_layer.h"
#include "rs_layerlist.h"

namespace {

class LayerStateProvider final : public LC_PropertiesProviderGraphicLayer {
public:
    LayerStateProvider() : LC_PropertiesProviderGraphicLayer(nullptr, nullptr) {}

    void fill(LC_PropertyContainer* container, RS_Graphic* graphic) {
        createVisible(graphic, container);
        createLocked(graphic, container);
    }
};

LC_PropertyBool* boolProperty(const LC_PropertyContainer& container, const QString& name) {
    const QList<LC_Property*> found = container.findChildProperties(name);
    return found.isEmpty() ? nullptr : dynamic_cast<LC_PropertyBool*>(found.constFirst());
}

} // namespace

TEST_CASE("The sheet's Visible layer is checked for a shown layer, as the dock's eye is open", "[gui][layers][properties][3229]") {
    (void)lc::test::application();
    RS_Graphic graphic;
    graphic.initForNewDocument();
    RS_Layer* walls = new RS_Layer(QStringLiteral("Walls"));
    graphic.addLayer(walls);
    graphic.activateLayer(walls);
    LayerStateProvider provider;
    LC_PropertyContainer container;
    provider.fill(&container, &graphic);
    LC_PropertyBool* visible = boolProperty(container, QStringLiteral("$layerVisible"));
    LC_PropertyBool* locked = boolProperty(container, QStringLiteral("$layerLocked"));
    REQUIRE(visible != nullptr);
    REQUIRE(locked != nullptr);

    CHECK(visible->value());
    CHECK_FALSE(locked->value());

    // a hidden layer can be current
    graphic.toggleLayer(walls);
    CHECK_FALSE(visible->value());
    CHECK_FALSE(locked->value());

    // hidden and locked, the two states apart: locking the current layer makes another layer current,
    // which is shown and not locked, and the sheet shows that one
    graphic.toggleLayerLock(walls);
    CHECK(walls->isLocked());
    CHECK(walls->isFrozen());
    CHECK(graphic.getActiveLayer() != walls);
    CHECK(visible->value());
    CHECK_FALSE(locked->value());
}

TEST_CASE("Checking Visible layer and Locked in the sheet shows and hides, locks and unlocks the layer", "[gui][layers][properties][3229]") {
    (void)lc::test::application();
    RS_Graphic graphic;
    graphic.initForNewDocument();
    RS_Layer* walls = new RS_Layer(QStringLiteral("Walls"));
    graphic.addLayer(walls);
    graphic.activateLayer(walls);
    LayerStateProvider provider;
    LC_PropertyContainer container;
    provider.fill(&container, &graphic);
    LC_PropertyBool* visible = boolProperty(container, QStringLiteral("$layerVisible"));
    LC_PropertyBool* locked = boolProperty(container, QStringLiteral("$layerLocked"));
    REQUIRE(visible != nullptr);
    REQUIRE(locked != nullptr);

    visible->setValue(false);
    CHECK(walls->isFrozen());
    CHECK(graphic.getActiveLayer() == walls);
    visible->setValue(true);
    CHECK_FALSE(walls->isFrozen());

    // locking the current layer: another layer, here "0", is current in its place
    locked->setValue(true);
    CHECK(walls->isLocked());
    CHECK(graphic.getActiveLayer() == graphic.findLayer(QStringLiteral("0")));
    CHECK_FALSE(locked->value());
}

TEST_CASE("The sheet's Locked is read-only when no other layer is unlocked, and says why", "[gui][layers][properties][3229][offer]") {
    // locking the current layer needs another layer that is not locked to take its place
    (void)lc::test::application();
    RS_Graphic graphic;
    graphic.initForNewDocument();
    LayerStateProvider provider;
    const auto locked = [&provider, &graphic](LC_PropertyContainer& container) {
        provider.fill(&container, &graphic);
        LC_PropertyBool* property = boolProperty(container, QStringLiteral("$layerLocked"));
        REQUIRE(property != nullptr);
        return property;
    };

    SECTION("the only layer of a drawing") {
        LC_PropertyContainer container;
        LC_PropertyBool* property = locked(container);
        CHECK_FALSE(property->isEditableByUser());
        CHECK(property->getDescription().contains(RS_LayerList::lockUnavailableText()));
        // the other boxes of the sheet are not affected
        LC_PropertyBool* visible = boolProperty(container, QStringLiteral("$layerVisible"));
        REQUIRE(visible != nullptr);
        CHECK(visible->isEditableByUser());
    }

    SECTION("another layer that is not locked") {
        RS_Layer* walls = new RS_Layer(QStringLiteral("Walls"));
        graphic.addLayer(walls);
        graphic.activateLayer(walls);
        LC_PropertyContainer container;
        LC_PropertyBool* property = locked(container);
        CHECK(property->isEditableByUser());
        CHECK_FALSE(property->getDescription().contains(RS_LayerList::lockUnavailableText()));

        // the sheet is filled again when a layer is locked: nothing is left to take the place of "Walls"
        graphic.toggleLayerLock(graphic.findLayer(QStringLiteral("0")));
        LC_PropertyContainer refilled;
        CHECK_FALSE(locked(refilled)->isEditableByUser());

        // and when one is unlocked
        graphic.toggleLayerLock(graphic.findLayer(QStringLiteral("0")));
        LC_PropertyContainer again;
        CHECK(locked(again)->isEditableByUser());
    }
}
