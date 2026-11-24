/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2026 LibreCAD.org
** Copyright (C) 2026 Dongxu Li (github.com/dxli)
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
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
**
****************************************************************************/

// The Insert panel of the property sheet: the Rows row that kept the wrong focus, the Block list that offered
// blocks a reference cannot expand, the count rows without an upper limit and a zero scale factor.

#include <catch2/catch_test_macros.hpp>

#include <string>

#include <QCoreApplication>
#include <QEventLoop>
#include <QTimer>

#include "lc_actioncontext.h"
#include "lc_actiongroupmanager.h"
#include "lc_actiontestsupport.h"
#include "lc_properties_sheet.h"
#include "lc_property_container.h"
#include "lc_property_double.h"
#include "lc_property_int.h"
#include "lc_property_int_spinbox_view.h"
#include "lc_property_multi.h"
#include "lc_property_qstring_list_combobox_view.h"
#include "lc_propertysheet_widget_options.h"
#include "lc_propertysheetwidget.h"
#include "rs_block.h"
#include "rs_graphic.h"
#include "rs_insert.h"
#include "rs_line.h"

namespace {

/// the row shown under this name, searched through all sections of the sheet
LC_Property* findByDisplayName(LC_PropertyContainer* container, const QString& displayName) {
    for (LC_Property* property : container->childProperties()) {
        if (property->getDisplayName() == displayName) {
            return property;
        }
        if (LC_PropertyContainer* section = property->asContainer()) {
            if (LC_Property* found = findByDisplayName(section, displayName)) {
                return found;
            }
        }
    }
    return nullptr;
}

/// runs the event loop for \p milliseconds (a property edit is applied by a 30 ms timer)
void waitMs(const int milliseconds) {
    QEventLoop loop;
    QTimer::singleShot(milliseconds, &loop, &QEventLoop::quit);
    loop.exec();
}

/// the one reference in \p container that has not been replaced by an edit
RS_Insert* liveInsert(RS_EntityContainer& container) {
    RS_Insert* found = nullptr;
    int count = 0;
    for (RS_Entity* e : container) {
        if (e != nullptr && e->rtti() == RS2::EntityInsert && !e->isDeleted()) {
            found = static_cast<RS_Insert*>(e);
            ++count;
        }
    }
    REQUIRE(count == 1);
    return found;
}

/// the application comes first: a drawing reads its settings when it is built
struct TestApplication {
    TestApplication() {
        (void)lc::test::application();
    }
};

/// a property sheet over a drawing with block B (a line from 0,0 to 10,0) and the layout block *Paper_Space
struct Sheet : TestApplication {
    LC_ActionContext context;
    LC_ActionGroupManager actions{nullptr};
    LC_PropertySheetWidget sheet{nullptr, &context, &actions}; // before the drawing, so that it is destroyed after it
    RS_Graphic graphic;
    lc::test::TestGraphicView view;

    Sheet() {
        sheet.getOptions()->noSelectionWorkspace = false;
        sheet.getOptions()->noSelectionGraphicView = false;
        graphic.initForNewDocument(); // the layer 0, which the sheet paints in its Layer row
        auto* block = new RS_Block(&graphic, RS_BlockData("B", RS_Vector(0, 0), false));
        block->addEntity(new RS_Line(block, RS_LineData(RS_Vector(0, 0), RS_Vector(10, 0))));
        graphic.addBlock(block);
        graphic.addBlock(new RS_Block(&graphic, RS_BlockData("*Paper_Space", RS_Vector(0, 0), false)));
    }

    ~Sheet() {
        sheet.setGraphicView(nullptr);
    }

    /// a reference of block B in \p container
    RS_Insert* addInsert(RS_EntityContainer* container) {
        auto* insert = new RS_Insert(container, RS_InsertData("B", RS_Vector(0, 0), RS_Vector(1, 1), 0., 1, 1, RS_Vector(0, 0)));
        container->addEntity(insert);
        insert->update();
        return insert;
    }

    /// shows \p selected, an entity of \p document, in the sheet
    void open(RS_Document* document, RS_Entity* selected) {
        document->select(QList<RS_Entity*>{selected}, true);
        view.setDocument(document);
        context.setDocumentAndView(document, &view);
        sheet.show();
        QCoreApplication::processEvents();
        sheet.setGraphicView(&view);
        QCoreApplication::processEvents();
    }

    LC_PropertiesSheet* properties() {
        auto* properties = sheet.findChild<LC_PropertiesSheet*>();
        REQUIRE(properties != nullptr);
        REQUIRE(properties->getPropertyContainer() != nullptr);
        return properties;
    }

    /// the row shown under this name: it wraps the property of each selected entity
    LC_PropertyMulti* row(const QString& name) {
        auto* multi = dynamic_cast<LC_PropertyMulti*>(findByDisplayName(properties()->getPropertyContainer(), name));
        INFO(name.toStdString());
        REQUIRE(multi != nullptr);
        return multi;
    }

    /// what the editor of a row does when the user enters \p value
    template <class PropertyClass, class Value>
    void edit(const QString& name, const Value value) {
        LC_PropertyMulti* multi = row(name);
        auto* typed = dynamic_cast<PropertyClass*>(multi->getFirstProperty());
        REQUIRE(typed != nullptr);
        REQUIRE(typed->setValue(value, LC_PropertyChangeReason(PropertyChangeReasonEdit) | PropertyChangeReasonValueNew));
        sheet.onPropertyEdited(multi);
        waitMs(100);
    }

    /// the names the Block row offers, joined (a QStringList prints as {?} in a failed check)
    std::string blockNames() {
        const LC_PropertyViewDescriptor* descriptor = row("Block")->getFirstProperty()->getViewDescriptor();
        REQUIRE(descriptor != nullptr);
        return descriptor->attributes.value(LC_PropertyQStringListComboBoxView::ATTR_ITEMS).toStringList().join(",").toStdString();
    }
};

}

TEST_CASE("The Rows row of a block reference keeps the focus when it is edited", "[gui][properties][insert]") {
    Sheet s;
    s.open(&s.graphic, s.addInsert(&s.graphic));
    // the sheet finds the active row again by its name
    CHECK(s.row("Rows")->getName().toStdString() != s.row("Columns")->getName().toStdString());
    s.properties()->setActiveProperty(s.row("Rows"), true);
    QCoreApplication::processEvents();

    s.edit<LC_PropertyInt>("Rows", 2);

    CHECK(liveInsert(s.graphic)->getRows() == 2);
    REQUIRE(s.properties()->getActiveProperty() != nullptr);
    CHECK(s.properties()->getActiveProperty()->getDisplayName().toStdString() == "Rows");
}

TEST_CASE("The Block list of a block reference leaves out layout blocks and removed blocks", "[gui][properties][insert]") {
    Sheet s;
    s.graphic.addBlock(new RS_Block(&s.graphic, RS_BlockData("*U1", RS_Vector(0, 0), false)));
    auto* removed = new RS_Block(&s.graphic, RS_BlockData("R", RS_Vector(0, 0), false));
    s.graphic.addBlock(removed);
    removed->setFlag(RS2::FlagDeleted); // Blocks > Remove keeps the block in the list, flagged, for undo
    s.open(&s.graphic, s.addInsert(&s.graphic));

    CHECK(s.blockNames() == "B");

    // a reference that already uses a layout block keeps showing it
    Sheet anonymous;
    anonymous.graphic.addBlock(new RS_Block(&anonymous.graphic, RS_BlockData("*U1", RS_Vector(0, 0), false)));
    auto* insert = new RS_Insert(&anonymous.graphic, RS_InsertData("*U1", RS_Vector(0, 0), RS_Vector(1, 1), 0., 1, 1, RS_Vector(0, 0)));
    anonymous.graphic.addEntity(insert);
    anonymous.open(&anonymous.graphic, insert);

    CHECK(anonymous.blockNames() == "*U1,B");
}

TEST_CASE("In a block's editor the Block list leaves out that block and the blocks that contain it", "[gui][properties][insert]") {
    Sheet s;
    auto* a = new RS_Block(&s.graphic, RS_BlockData("A", RS_Vector(0, 0), false));
    s.graphic.addBlock(a);
    auto* c = new RS_Block(&s.graphic, RS_BlockData("C", RS_Vector(0, 0), false));
    s.graphic.addBlock(c);
    c->addEntity(new RS_Insert(c, RS_InsertData("A", RS_Vector(0, 0), RS_Vector(1, 1), 0., 1, 1, RS_Vector(0, 0))));
    s.graphic.addBlock(new RS_Block(&s.graphic, RS_BlockData("D", RS_Vector(0, 0), false)));
    s.open(a, s.addInsert(a)); // A is open, and holds a reference of B

    // a reference of A inside A, or of C (which holds A), would contain itself
    CHECK(s.blockNames() == "B,D");
}

TEST_CASE("The Columns and Rows rows stop at the largest array a DXF file reads back", "[gui][properties][insert]") {
    Sheet s;
    s.open(&s.graphic, s.addInsert(&s.graphic));

    for (const char* name : {"Columns", "Rows"}) {
        INFO(name);
        const LC_PropertyViewDescriptor* descriptor = s.row(name)->getFirstProperty()->getViewDescriptor();
        REQUIRE(descriptor != nullptr);
        // libdxfrw refuses the whole file when a count in groups 70 and 71 is above 32767
        CHECK(descriptor->attributes.value(LC_PropertyIntSpinBoxView::ATTR_MAX).toInt() == 32767);
    }
}

TEST_CASE("A zero scale factor in the Scale X row keeps the block reference expanded", "[gui][properties][insert]") {
    Sheet s;
    s.open(&s.graphic, s.addInsert(&s.graphic));

    s.edit<LC_PropertyDouble>("Scale X", 0.0); // the row's spin box takes a zero

    const RS_Insert* insert = liveInsert(s.graphic);
    CHECK(insert->getScale().x == 1.0);
    CHECK(insert->count() == 1);
}
