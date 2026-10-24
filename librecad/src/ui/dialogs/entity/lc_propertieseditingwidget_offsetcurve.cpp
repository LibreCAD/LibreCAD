/*
 * This file is part of LibreCAD, a 2D CAD program.
 * Copyright (C) 2026 LibreCAD.org
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "lc_propertieseditingwidget_offsetcurve.h"

#include <QCheckBox>
#include <QLabel>
#include <QLineEdit>
#include <QScopedValueRollback>
#include <QSignalBlocker>
#include <algorithm>
#include <cmath>
#include "lc_busycursorguard.h"
#include "lc_graphicviewport.h"
#include "lc_offsetcurveui.h"
#include "lc_offsetcurvereadutils.h"
#include "lc_pointpickbutton.h"
#include "rs_math.h"
#include "ui_lc_propertieseditingwidget_offsetcurve.h"

LC_PropertiesEditingWidgetOffsetCurve::LC_PropertiesEditingWidgetOffsetCurve(QWidget* parent)
    : LC_EntityPropertiesEditorWidget(parent), ui(new Ui::LC_OffsetCurvePropertiesEditingWidget),
      m_limits(LC_OffsetCurveUI::sheetLimits()) {
    ui->setupUi(this);
    const auto bind = [this](QLineEdit* editor, Field field) {
        connect(editor, &QLineEdit::editingFinished, this,
            [this, editor, field] { finishField(editor, field); });
    };
    bind(ui->leDistance, Field::Distance);
    bind(ui->leCenterX, Field::CenterX); bind(ui->leCenterY, Field::CenterY);
    bind(ui->leMajor, Field::Major); bind(ui->leMinor, Field::Minor);
    bind(ui->leRotation, Field::Rotation);
    bind(ui->leAngle1, Field::Start); bind(ui->leAngle2, Field::End);
    connect(ui->cbTowardCentre, &QCheckBox::toggled, this, [this](bool inward) {
        if (m_refreshing || m_entity == nullptr) return;
        apply([&] { return m_entity->setTowardCentre(inward, m_limits); });
    });
    connect(ui->cbReversed, &QCheckBox::toggled, this, [this](bool reversed) {
        if (m_refreshing || m_entity == nullptr) return;
        auto basis = m_entity->definition().basis;
        if (basis.reversed != reversed) {
            if (!m_entity->sourceIsWhole()) std::swap(basis.angle1, basis.angle2);
            basis.reversed = reversed;
        }
        apply([&] { return m_entity->setBasis(basis, m_limits); });
    });
}

LC_PropertiesEditingWidgetOffsetCurve::~LC_PropertiesEditingWidgetOffsetCurve() { delete ui; }

void LC_PropertiesEditingWidgetOffsetCurve::setEntity(RS_Entity* entity) {
    m_entity = entity != nullptr && entity->rtti() == RS2::EntityOffsetCurve
        ? static_cast<LC_OffsetCurve*>(entity) : nullptr;
    m_lastResult.reset(); m_attempts = 0;
    ui->lRefusal->clear();
    refresh(nullptr, true);
}

void LC_PropertiesEditingWidgetOffsetCurve::refresh(QLineEdit* completed, bool force) const {
    const QScopedValueRollback<bool> refreshing(m_refreshing, true);
    const bool available = m_entity != nullptr && m_entity->hasEditableDefinition() && m_viewport != nullptr;
    const auto text = [completed, force](QLineEdit* editor, const QString& value) {
        // Another dirty editor belongs to the user, not to this field's refresh.
        if (force || editor == completed || !editor->isModified()) {
            const QSignalBlocker blocked(editor);
            editor->setText(value);
            editor->setProperty("_offsetCurveLoadedText", value);
            editor->setModified(false);
        }
    };
    ui->gbDefinition->setEnabled(available);
    if (m_entity == nullptr || m_viewport == nullptr) return;
    const auto& definition = m_entity->definition();
    const auto& basis = definition.basis;
    const auto center = toUIStr(basis.center);
    text(ui->leDistance, asString(definition.distance));
    text(ui->leCenterX, center.first); text(ui->leCenterY, center.second);
    text(ui->leMajor, asString(basis.majorP.magnitude()));
    text(ui->leMinor, asString(basis.majorP.magnitude() * basis.ratio));
    text(ui->leRotation, toUIAngleDeg(basis.majorP.angle()));
    text(ui->leAngle1, asStringAngleDeg(basis.angle1));
    text(ui->leAngle2, asStringAngleDeg(basis.angle2));
    {
        const QSignalBlocker side(ui->cbTowardCentre), reversed(ui->cbReversed);
        ui->cbTowardCentre->setChecked(definition.towardCentre);
        ui->cbReversed->setChecked(basis.reversed);
    }
    const bool arc = available && !m_entity->sourceIsWhole();
    ui->leAngle1->setEnabled(arc); ui->leAngle2->setEnabled(arc);
    const auto length = LC_OffsetCurveRead::cachedLength(*m_entity);
    ui->lLengthValue->setText(length ? asString(*length) : tr("Unavailable"));
    ui->lState->setText(!m_entity->isDefined() ? tr("Undefined") :
        !m_entity->isConsistent() ? tr("Inconsistent") : tr("Defined"));
    if (!available) {
        LC_OffsetCurveEditResult state;
        state.refusal = !m_entity->isDefined()
            ? LC_OffsetCurveRefusal::Undefined : LC_OffsetCurveRefusal::Inconsistent;
        ui->lRefusal->setText(LC_OffsetCurveUI::refusalText(state));
    }
}

void LC_PropertiesEditingWidgetOffsetCurve::inputRefused(QLineEdit* editor, const QString& reason) const {
    m_lastResult.reset(); // no native setter was called for a parse/UI-domain failure
    refresh(editor);
    ui->lRefusal->setText(reason);
}

void LC_PropertiesEditingWidgetOffsetCurve::finishField(QLineEdit* editor, Field field) const {
    if (m_refreshing || m_entity == nullptr || !editor->isModified()) return;
    if (editor->text() == editor->property("_offsetCurveLoadedText").toString()) {
        editor->setModified(false); // never parse a rounded, unchanged display back into the definition
        return;
    }
    const bool angle = field == Field::Rotation || field == Field::Start || field == Field::End;
    double value = 0;
    bool valid = false;
    if (angle) valid = toDoubleAngle(editor->text(), value, 0., false);
    else value = RS_Math::eval(editor->text(), &valid);
    if (!valid || !std::isfinite(value)) {
        inputRefused(editor, tr("Enter a finite numeric expression; the value was not changed."));
        return;
    }
    // Consume editingFinished before rendering dependent fields. Return plus
    // focus-out must not submit a second edit or retain a stale dirty value.
    editor->setModified(false);
    applyValue(field, angle ? RS_Math::deg2rad(value) : value);
    refresh(editor);
}

void LC_PropertiesEditingWidgetOffsetCurve::apply(const std::function<LC_OffsetCurveEditResult()>& setter) const {
    if (m_entity == nullptr) return;
    {
        const LC_BusyCursorGuard busy;
        ++m_attempts;
        m_lastResult = setter(); // retain the actual producer/checker accounting, including refused work
    }
    refresh();
    ui->lRefusal->setText(*m_lastResult ? QString{} : LC_OffsetCurveUI::refusalText(*m_lastResult));
}

void LC_PropertiesEditingWidgetOffsetCurve::applyValue(Field field, double value) const {
    if (m_entity == nullptr || m_viewport == nullptr) return;
    auto basis = m_entity->definition().basis;
    if (!m_entity->hasEditableDefinition()) {
        // Let D-edit classify stale/undefined state before local radius checks.
        apply([&] { return m_entity->setBasis(basis, m_limits); });
        return;
    }
    if (field == Field::Distance) {
        apply([&] { return m_entity->setDistance(value, m_limits); });
        return;
    }
    const double major = basis.majorP.magnitude(), minor = major * basis.ratio;
    switch (field) {
    case Field::CenterX:
    case Field::CenterY: {
        auto center = toUCSVector(basis.center);
        double& component = field == Field::CenterX ? center.x : center.y;
        if (component != value) {
            component = value;
            basis.center = toWCSVector(center);
        }
        break;
    }
    case Field::Major:
        if (!(value > 0) || value < minor) {
            LC_OffsetCurveEditResult invalid;
            inputRefused(ui->leMajor, LC_OffsetCurveUI::refusalText(invalid)); return;
        }
        if (value != major) {
            basis.majorP = basis.majorP * (value / major);
            basis.ratio = minor / value;
        }
        break;
    case Field::Minor:
        if (!(value > 0) || value > major) {
            LC_OffsetCurveEditResult invalid;
            inputRefused(ui->leMinor, LC_OffsetCurveUI::refusalText(invalid)); return;
        }
        if (value != minor) basis.ratio = value / major;
        break;
    case Field::Rotation:
        if (value != toUCSAngle(basis.majorP.angle())) {
            const double world = m_viewport->toWorldAngle(m_viewport->toAbsUCSAngle(value));
            basis.majorP = RS_Vector::polar(major, world);
        }
        break;
    case Field::Start: basis.angle1 = value; break;
    case Field::End: basis.angle2 = value; break;
    case Field::Distance: break;
    }
    apply([&] { return m_entity->setBasis(basis, m_limits); });
}

void LC_PropertiesEditingWidgetOffsetCurve::setupInteractiveInputWidgets() {
    pickPointSetup(ui->wPickPointCenter, "center", ui->leCenterX, ui->leCenterY);
    pickDistanceSetup(ui->tbPickDistance, "distance", ui->leDistance);
    pickDistanceSetup(ui->tbPickMajor, "major", ui->leMajor);
    pickDistanceSetup(ui->tbPickMinor, "minor", ui->leMinor);
    pickAngleSetup(ui->tbPickRotation, "rotation", ui->leRotation);
    // Raw ellipse parameters are not WCS bearings: deliberately no angle picker.
}

void LC_PropertiesEditingWidgetOffsetCurve::interactiveInputUpdate(
    LC_ActionContext::InteractiveInputInfo::InputType inputType, const QString& tag,
    double valueOne, double valueTwo) const {
    using Input = LC_ActionContext::InteractiveInputInfo;
    if (m_entity == nullptr) return;
    if (inputType == Input::POINT && tag == "center") {
        auto basis = m_entity->definition().basis;
        basis.center = RS_Vector{valueOne, valueTwo, basis.center.z};
        apply([&] { return m_entity->setBasis(basis, m_limits); });
    }
    else if (inputType == Input::DISTANCE) {
        if (tag == "distance") applyValue(Field::Distance, valueOne);
        else if (tag == "major") applyValue(Field::Major, valueOne);
        else if (tag == "minor") applyValue(Field::Minor, valueOne);
    }
    else if (inputType == Input::ANGLE && tag == "rotation") {
        // PickAngle returns the same UCS/basis radians that the rotation editor displays.
        applyValue(Field::Rotation, valueOne);
    }
}
