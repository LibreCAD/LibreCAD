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

// The Insert Properties dialog (Modify > Properties on a block reference): the Columns field that was wired to the
// Rows slot, the crossed spacing pick buttons, and the counts and the scale factor that emptied the reference.

#include <catch2/catch_test_macros.hpp>

#include <memory>

#include <QLineEdit>
#include <QStringList>
#include <QToolButton>

#include "lc_actiontestsupport.h"
#include "lc_propertieseditingwidget_insert.h"
#include "rs_block.h"
#include "rs_graphic.h"
#include "rs_insert.h"
#include "rs_line.h"

namespace {

/// the application comes first: a drawing reads its settings when it is built
struct TestApplication {
    TestApplication() {
        (void)lc::test::application();
    }
};

/// the dialog's panel over a clone of a reference of block B (a line from 0,0 to 10,0): the clone is what it edits
struct Dialog : TestApplication {
    RS_Graphic graphic;
    lc::test::TestGraphicView view;
    RS_Insert* clone = nullptr;
    std::unique_ptr<LC_PropertiesEditingWidgetInsert> widget;

    Dialog() {
        graphic.initForNewDocument();
        auto* block = new RS_Block(&graphic, RS_BlockData("B", RS_Vector(0, 0), false));
        block->addEntity(new RS_Line(block, RS_LineData(RS_Vector(0, 0), RS_Vector(10, 0))));
        graphic.addBlock(block);
        auto* insert = new RS_Insert(&graphic, RS_InsertData("B", RS_Vector(0, 0), RS_Vector(1, 1), 0., 1, 1, RS_Vector(0, 0)));
        graphic.addEntity(insert);
        insert->update();
        view.setDocument(&graphic);

        clone = static_cast<RS_Insert*>(insert->clone());
        widget = std::make_unique<LC_PropertiesEditingWidgetInsert>(nullptr);
        widget->setGraphicViewport(view.getViewPort());
        widget->setEntity(clone);
        static_cast<LC_EntityPropertiesEditorWidget&>(*widget).setupInteractiveInputWidgets(); // the dialog calls it when it opens
    }

    ~Dialog() {
        widget.reset();
        delete clone;
    }

    QLineEdit* field(const char* name) const {
        auto* edit = widget->findChild<QLineEdit*>(name);
        REQUIRE(edit != nullptr);
        return edit;
    }

    /// what the user does to a field: enters \p text and leaves it
    void type(const char* name, const char* text) const {
        QLineEdit* edit = field(name);
        edit->setText(QString::fromLatin1(text));
        REQUIRE(QMetaObject::invokeMethod(edit, "editingFinished"));
    }
};

}

TEST_CASE("The Insert Properties dialog applies the Columns field", "[gui][dialog][insert]") {
    Dialog d;

    d.type("leCols", "4");

    CHECK(d.clone->getCols() == 4);
    CHECK(d.clone->getRows() == 1);
}

TEST_CASE("Each spacing pick button of the Insert Properties dialog asks for its own field", "[gui][dialog][insert]") {
    Dialog d;
    QStringList asked;
    QObject::connect(d.widget.get(), &LC_EntityPropertiesEditorWidget::interactiveInputRequested,
                     [&asked](LC_ActionContext::InteractiveInputInfo::InputType, const QString& tag) {
                         asked.append(tag);
                     });
    auto* rowButton = d.widget->findChild<QToolButton*>("tbPickRowSpacing");
    auto* columnButton = d.widget->findChild<QToolButton*>("tbPickColumnSpacing");
    REQUIRE(rowButton != nullptr);
    REQUIRE(columnButton != nullptr);

    rowButton->click();
    CHECK(asked.join(",").toStdString() == "rowSpacing");

    asked.clear();
    columnButton->click();
    CHECK(asked.join(",").toStdString() == "colSpacing");
}

TEST_CASE("The Insert Properties dialog keeps the array counts it cannot use", "[gui][dialog][insert]") {
    // below 1 the reference is empty and Save as DXF fails; above 32767 the saved DXF file cannot be opened
    for (const char* text : {"0", "-2", "abc", "", "32768"}) {
        Dialog d;
        INFO("typed '" << text << "'");

        d.type("leRows", text);
        d.type("leCols", text);

        CHECK(d.clone->getRows() == 1);
        CHECK(d.clone->getCols() == 1);
        CHECK(d.field("leRows")->text().toStdString() == "1");
        CHECK(d.field("leCols")->text().toStdString() == "1");
    }

    Dialog d;
    d.type("leRows", "32767");
    CHECK(d.clone->getRows() == 32767);
}

TEST_CASE("The Insert Properties dialog keeps a zero scale factor from emptying the reference", "[gui][dialog][insert]") {
    for (const char* text : {"0", "1/0"}) {
        Dialog d;
        INFO("typed '" << text << "' in Scale X");

        d.type("leScaleX", text);
        d.clone->update();

        CHECK(d.clone->getScale().x == 1.0);
        CHECK(d.clone->count() == 1);
    }
}
