/****************************************************************************
** Copyright (C) 2026 LibreCAD (librecad.org)
** SPDX-License-Identifier: GPL-2.0-or-later
****************************************************************************/
#include <QCoreApplication>
#include <QDir>
#include <QElapsedTimer>
#include <QMetaObject>
#include <QTemporaryDir>
#include <catch2/catch_test_macros.hpp>
#include <iostream>
#include <vector>

#include "lc_actioncontext.h"
#include "lc_actiongroupmanager.h"
#include "lc_actiontestsupport.h"
#include "lc_linetype.h"
#include "lc_properties_sheet.h"
#include "lc_property_container.h"
#include "lc_property_linetype.h"
#include "lc_property_linetype_combobox.h"
#include "lc_property_linetype_combobox_view.h"
#include "lc_property_linewidth.h"
#include "lc_property_multi.h"
#include "lc_propertysheet_widget_options.h"
#include "lc_propertysheetwidget.h"
#include "lc_testtempdirectory.h"
#include "rs_filterdxfrw.h"
#include "rs_graphic.h"
#include "rs_line.h"

namespace {
    LC_Property* findRow(LC_PropertyContainer* container, const QString& name) {
        for (LC_Property* property : container->childProperties()) {
            if (property->getDisplayName() == name)
                return property;
            if (auto* section = property->asContainer())
                if (auto* found = findRow(section, name))
                    return found;
        }
        return nullptr;
    }
    LC_LineTypeIdentity named(const QString& name) {
        RS_Pen pen;
        pen.setLineTypeName(name);
        return LC_LineTypeIdentity::fromPen(pen);
    }
    void waitForEdit() {
        QElapsedTimer timer;
        timer.start();
        while (timer.elapsed() < 150)
            QCoreApplication::processEvents(QEventLoop::AllEvents, 20);
    }
    struct TestApplication {
        TestApplication() {
            (void)lc::test::application();
        }
    };
    struct Sheet : TestApplication {
        LC_ActionContext context;
        LC_ActionGroupManager actions{nullptr};
        LC_PropertySheetWidget sheet{nullptr, &context, &actions};
        RS_Graphic graphic;
        lc::test::TestGraphicView view;
        Sheet() {
            sheet.getOptions()->noSelectionWorkspace = false;
            sheet.getOptions()->noSelectionGraphicView = false;
            graphic.initForNewDocument();
            for (const auto& name : {QStringLiteral("CUSTOM_A"), QStringLiteral("CUSTOM_B")}) {
                auto* type = new LC_LineType(name);
                type->pattern = {4.0, -2.0};
                graphic.addLineType(type);
            }
        }
        ~Sheet() {
            sheet.setGraphicView(nullptr);
        }
        RS_Line* addLine(double y, const QString& name) {
            auto* line = new RS_Line(&graphic, RS_LineData(RS_Vector(0, y), RS_Vector(100, y)));
            RS_Pen pen = line->getPen(false);
            pen.setLineTypeName(name);
            line->setPen(pen);
            graphic.addEntity(line);
            return line;
        }
        void open(const QList<RS_Entity*>& selected) {
            graphic.select(selected, true);
            view.setDocument(&graphic);
            context.setDocumentAndView(&graphic, &view);
            sheet.show();
            QCoreApplication::processEvents();
            sheet.setGraphicView(&view);
            QCoreApplication::processEvents();
        }
        LC_PropertyMulti* row(const QString& name) {
            auto* properties = sheet.findChild<LC_PropertiesSheet*>();
            REQUIRE(properties != nullptr);
            REQUIRE(properties->getPropertyContainer() != nullptr);
            auto* result = dynamic_cast<LC_PropertyMulti*>(findRow(properties->getPropertyContainer(), name));
            REQUIRE(result != nullptr);
            return result;
        }
        template <typename Property, typename Value>
        void edit(const QString& name, const Value& value) {
            auto* item = row(name);
            auto* property = dynamic_cast<Property*>(item->getFirstProperty());
            REQUIRE(property != nullptr);
            auto reason = LC_PropertyChangeReason(PropertyChangeReasonEdit) | PropertyChangeReasonValueNew;
            if (item->isMultiValue())
                reason |= PropertyChangeReasonMultiEdit;
            item->markEdited(true);
            REQUIRE(property->setValue(value, reason));
            sheet.onPropertyEdited(item);
            waitForEdit();
        }
        std::vector<RS_Line*> lines() {
            std::vector<RS_Line*> result;
            for (unsigned i = 0; i < graphic.count(); ++i)
                if (auto* line = dynamic_cast<RS_Line*>(graphic.entityAt(i)); line && !line->isDeleted())
                    result.push_back(line);
            return result;
        }
    };
} // namespace

TEST_CASE("Named linetype selector retains names and offers drawing patterns", "[gui][properties][named-linetype]") {
    (void)lc::test::application();
    RS_Graphic graphic;
    graphic.initForNewDocument();
    auto* custom = new LC_LineType(QStringLiteral("CUSTOM_A"));
    custom->pattern = {4.0, -2.0};
    graphic.addLineType(custom);
    auto* other = new LC_LineType(QStringLiteral("CUSTOM_B"));
    other->pattern = {3.0, -1.0};
    graphic.addLineType(other);
    LC_PropertyNamedLineType property(nullptr);
    property.setLineTypeList(&graphic.getLineTypeList());
    REQUIRE(property.setValue(named(QStringLiteral("CUSTOM_A"))));
    LC_PropertyNamedLineTypeComboboxView view(&property);
    LC_PropertyNamedLineTypeCombobox combo(&view, nullptr);
    RS_Pen selected;
    combo.applyLineTypeTo(selected);
    CHECK(selected.getLineTypeName() == QStringLiteral("CUSTOM_A"));
    const auto definition = combo.definitionOf(selected);
    REQUIRE(definition.has_value());
    CHECK((definition->pattern == std::vector<double>{4.0, -2.0}));
    const int index = combo.findText(QStringLiteral("CUSTOM_B"));
    REQUIRE(index >= 0);
    combo.setCurrentIndex(index);
    REQUIRE(QMetaObject::invokeMethod(&combo, "activated", Qt::DirectConnection, Q_ARG(int, index)));
    CHECK(property.value() == named(QStringLiteral("CUSTOM_B")));
    combo.setCurrentIndex(0);
    REQUIRE(QMetaObject::invokeMethod(&combo, "activated", Qt::DirectConnection, Q_ARG(int, 0)));
    CHECK(property.value().type == RS2::LineByLayer);
    CHECK_FALSE(property.value().hasName);
    LC_PropertyNamedLineType detached(nullptr);
    REQUIRE(detached.setValue(named(QStringLiteral("MISSING_CUSTOM"))));
    LC_PropertyNamedLineTypeComboboxView detachedView(&detached);
    LC_PropertyNamedLineTypeCombobox detachedCombo(&detachedView, nullptr);
    RS_Pen detachedPen;
    detachedCombo.applyLineTypeTo(detachedPen);
    CHECK(detachedPen.getLineTypeName() == QStringLiteral("MISSING_CUSTOM"));
    RS_Pen noPen;
    noPen.setLineType(RS2::NoPen);
    LC_PropertyNamedLineType none(nullptr);
    REQUIRE(none.setValue(LC_LineTypeIdentity::fromPen(noPen)));
    LC_PropertyNamedLineTypeComboboxView noneView(&none);
    LC_PropertyNamedLineTypeCombobox noneCombo(&noneView, nullptr);
    RS_Pen selectedNone;
    noneCombo.applyLineTypeTo(selectedNone);
    CHECK(selectedNone.getLineType() == RS2::NoPen);
    CHECK_FALSE(selectedNone.hasLineTypeName());
    // An unnamed built-in and a named record can render differently when a
    // drawing redefines that name, even though their enum values match.
    RS_Pen stock;
    stock.setLineType(RS2::DashLine);
    RS_Pen record;
    record.setLineTypeName(QStringLiteral("DASHED"));
    CHECK_FALSE(LC_LineTypeIdentity::fromPen(stock) == LC_LineTypeIdentity::fromPen(record));

    // Choosing the first selected value must still propagate it when the
    // selected entities have different names that share the SolidLine enum.
    LC_PropertyNamedLineType first(nullptr), second(nullptr);
    first.setLineTypeList(&graphic.getLineTypeList());
    REQUIRE(first.setValue(named(QStringLiteral("CUSTOM_A"))));
    REQUIRE(second.setValue(named(QStringLiteral("CUSTOM_B"))));
    LC_PropertyMulti multi(&LC_PropertyNamedLineType::staticMetaObject, nullptr);
    multi.addProperty(&first, false);
    multi.addProperty(&second, false);
    multi.updateMultipleState(true);
    REQUIRE(multi.isMultiValue());
    multi.markEdited(true);
    LC_PropertyNamedLineTypeComboboxView multiView(&first);
    multiView.setStateProperty(&multi);
    LC_PropertyNamedLineTypeCombobox multiCombo(&multiView, nullptr);
    const int firstIndex = multiCombo.findText(QStringLiteral("CUSTOM_A"));
    REQUIRE(firstIndex >= 0);
    multiCombo.setCurrentIndex(firstIndex);
    REQUIRE(QMetaObject::invokeMethod(&multiCombo, "activated", Qt::DirectConnection, Q_ARG(int, firstIndex)));
    CHECK(first.value() == named(QStringLiteral("CUSTOM_A")));
    CHECK(second.value() == named(QStringLiteral("CUSTOM_A")));
}

TEST_CASE("Property sheet preserves and edits a named linetype", "[gui][properties][named-linetype]") {
    Sheet s;
    s.open({s.addLine(0, QStringLiteral("CUSTOM_A"))});
    auto* row = s.row(QStringLiteral("Line Type"));
    auto* name = dynamic_cast<LC_PropertyNamedLineType*>(row->getFirstProperty());
    REQUIRE(name != nullptr);
    CHECK(name->value() == named(QStringLiteral("CUSTOM_A")));
    s.edit<LC_PropertyLineWidth>(QStringLiteral("Line Width"), RS2::Width05);
    REQUIRE(s.lines().size() == 1u);
    CHECK(s.lines()[0]->getPen(false).getLineTypeName() == QStringLiteral("CUSTOM_A"));
    s.edit<LC_PropertyNamedLineType>(QStringLiteral("Line Type"), named(QStringLiteral("CUSTOM_B")));
    REQUIRE(s.lines().size() == 1u);
    CHECK(s.lines()[0]->getPen(false).getLineTypeName() == QStringLiteral("CUSTOM_B"));
    const bool keepOutput = qEnvironmentVariableIntValue("LIBRECAD_KEEP_NAMED_LTYPE_OUTPUT") == 1;
    const auto tempRoot = keepOutput ? QString::fromStdString(lc::test::sharedTempDirectory().string()) : QDir::tempPath();
    QTemporaryDir temp(QDir(tempRoot).filePath(QStringLiteral("librecad-named-ltype-XXXXXX")));
    REQUIRE(temp.isValid());
    const auto path = temp.filePath(QStringLiteral("named.dxf"));
    if (keepOutput) {
        temp.setAutoRemove(false);
        s.sheet.resize(640, 600);
        QCoreApplication::processEvents();
        const auto screenshot = temp.filePath(QStringLiteral("properties.png"));
        CHECK(s.sheet.grab().save(screenshot));
        std::cerr << "named-linetype-output=" << path.toStdString() << '\n';
        std::cerr << "named-linetype-screenshot=" << screenshot.toStdString() << '\n';
    }
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileExport(s.graphic, path, RS2::FormatDXFRW));
    RS_Graphic reread;
    REQUIRE(filter.fileImport(reread, path, RS2::FormatDXFRW));
    const auto* type = reread.findLineType(QStringLiteral("CUSTOM_B"));
    REQUIRE(type != nullptr);
    CHECK((type->pattern == std::vector<double>{4.0, -2.0}));
    bool found = false;
    for (unsigned i = 0; i < reread.count(); ++i)
        if (auto* line = dynamic_cast<RS_Line*>(reread.entityAt(i)); line && !line->isDeleted()) {
            CHECK(line->getPen(false).getLineTypeName() == QStringLiteral("CUSTOM_B"));
            found = true;
        }
    CHECK(found);
}

TEST_CASE("Property sheet distinguishes and applies multiple custom linetypes", "[gui][properties][named-linetype]") {
    Sheet s;
    auto* a = s.addLine(0, QStringLiteral("CUSTOM_A"));
    auto* b = s.addLine(10, QStringLiteral("CUSTOM_B"));
    s.open({a, b});
    auto* row = s.row(QStringLiteral("Line Type"));
    CHECK(row->isMultiValue());
    s.edit<LC_PropertyNamedLineType>(QStringLiteral("Line Type"), named(QStringLiteral("CUSTOM_A")));
    REQUIRE(s.lines().size() == 2u);
    for (const auto* line : s.lines())
        CHECK(line->getPen(false).getLineTypeName() == QStringLiteral("CUSTOM_A"));
}

TEST_CASE("Property sheet preserves an unnamed No Pen value", "[gui][properties][named-linetype]") {
    Sheet s;
    auto* line = s.addLine(0, QStringLiteral("CUSTOM_A"));
    RS_Pen pen = line->getPen(false);
    pen.setLineType(RS2::NoPen);
    line->setPen(pen);
    s.open({line});
    auto* row = s.row(QStringLiteral("Line Type"));
    auto* type = dynamic_cast<LC_PropertyNamedLineType*>(row->getFirstProperty());
    REQUIRE(type != nullptr);
    CHECK(type->value().type == RS2::NoPen);
    CHECK_FALSE(type->value().hasName);
    s.edit<LC_PropertyLineWidth>(QStringLiteral("Line Width"), RS2::Width05);
    REQUIRE(s.lines().size() == 1u);
    CHECK(s.lines()[0]->getPen(false).getLineType() == RS2::NoPen);
    CHECK_FALSE(s.lines()[0]->getPen(false).hasLineTypeName());
}
