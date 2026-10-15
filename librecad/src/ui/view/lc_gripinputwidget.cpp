// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 LibreCAD contributors
#include "lc_gripinputwidget.h"

#include <QGridLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QStyle>
#include <cmath>
#include <algorithm>
#include "lc_formatter.h"
#include "lc_graphicviewport.h"
#include "rs_math.h"

LC_GripInputWidget::LC_GripInputWidget(QWidget* parent, LC_GraphicViewport* viewport)
    : QFrame(parent), m_viewport(viewport) {
    setObjectName("cad_grip_input");
    setFrameShape(QFrame::StyledPanel);
    setAutoFillBackground(true);
    auto* layout = new QGridLayout(this);
    layout->setContentsMargins(6, 4, 6, 4);
    layout->setSpacing(4);
    m_lengthCaption = new QLabel(this);
    auto* angleCaption = new QLabel(tr("Angle (°)"), this);
    m_length = new QLineEdit(this);
    m_angle = new QLineEdit(this);
    m_length->setObjectName("cad_grip_length");
    m_angle->setObjectName("cad_grip_angle");
    m_length->setAccessibleName(tr("Grip length in drawing units"));
    m_angle->setAccessibleName(tr("Grip angle in UCS degrees"));
    m_length->setMinimumWidth(80);
    m_angle->setMinimumWidth(80);
    m_length->setToolTip(tr("Drawing units. Enter a value to lock it; clear to unlock. Tab changes field."));
    m_angle->setToolTip(tr("Degrees relative to the current UCS and angle basis. Clear to unlock."));
    layout->addWidget(m_lengthCaption, 0, 0);
    layout->addWidget(angleCaption, 0, 1);
    layout->addWidget(m_length, 1, 0);
    layout->addWidget(m_angle, 1, 1);
    for (auto* field : {m_length, m_angle}) {
        field->installEventFilter(this);
        connect(field, &QLineEdit::textEdited, this, [this, field] {
            (field == m_length ? m_lengthLocked : m_angleLocked) = !field->text().trimmed().isEmpty();
            double number = 0;
            field->setProperty("invalid", !field->text().isEmpty() && !value(field, number, field == m_length));
            field->style()->unpolish(field);
            field->style()->polish(field);
            emit previewChanged();
        });
    }
    setStyleSheet("QLineEdit { border: 1px solid palette(mid); padding: 2px; }"
                  "QLineEdit:focus { border-color: palette(highlight); }"
                  "QLineEdit[invalid=\"true\"] { border: 2px solid #d84e4e; }");
    hide();
}

bool LC_GripInputWidget::value(QLineEdit* field, double& result, bool positive) const {
    bool ok = false;
    result = RS_Math::eval(field->text().trimmed(), &ok);
    return ok && std::isfinite(result) && (!positive || (std::isfinite(result * result) &&
           (m_dimensionLength ? result > RS_TOLERANCE : result >= 0)));
}

bool LC_GripInputWidget::inputValid() const {
    double number = 0;
    return (!m_lengthLocked || value(m_length, number, true)) && (!m_angleLocked || value(m_angle, number, false));
}

void LC_GripInputWidget::begin(const RS_Vector& anchor, const RS_Vector& point,
                             const QString& lengthCaption, bool reversed) {
    m_anchor = anchor;
    m_planeZ = point.z;
    m_dimensionLength = anchor.distanceTo(point) > RS_TOLERANCE;
    m_reversed = reversed;
    m_lengthLocked = m_angleLocked = false;
    m_lengthCaption->setText(lengthCaption);
    for (auto* field : {m_length, m_angle}) {
        field->setProperty("invalid", false);
        field->style()->unpolish(field);
        field->style()->polish(field);
    }
    updatePoint(point);
    show();
    raise();
    focusLength();
}

RS_Vector LC_GripInputWidget::constrain(const RS_Vector& point) const {
    if (!point.valid || !inputValid()) return point;
    if (!m_lengthLocked && !m_angleLocked) return point;
    const auto delta = m_reversed ? m_anchor - point : point - m_anchor;
    double length = std::hypot(delta.x, delta.y), angle = delta.angle();
    if (m_lengthLocked) value(m_length, length, true);
    if (m_angleLocked) {
        double degrees = 0;
        value(m_angle, degrees, false);
        angle = m_viewport->getFormatter()->toWorldAngleFromUCSBasisDegrees(std::fmod(degrees, 360.0));
    }
    const auto offset = RS_Vector::polar(length, angle);
    auto result = m_reversed ? m_anchor - offset : m_anchor + offset;
    result.z = m_planeZ; // 2D dimensional input retains the grip's elevation metadata.
    return result;
}

void LC_GripInputWidget::updatePoint(const RS_Vector& point) {
    const auto constrained = constrain(point);
    const auto delta = m_reversed ? m_anchor - constrained : constrained - m_anchor;
    if (!m_lengthLocked && !(m_length->hasFocus() && m_length->text().isEmpty())) {
        m_length->setText(QString::number(std::hypot(delta.x, delta.y), 'g', 10));
        if (m_length->hasFocus()) m_length->selectAll();
    }
    if (!m_angleLocked && !(m_angle->hasFocus() && m_angle->text().isEmpty())) {
        m_angle->setText(QString::number(m_viewport->getFormatter()->toUCSBasisAngleDegrees(delta.angle()), 'g', 10));
        if (m_angle->hasFocus()) m_angle->selectAll();
    }
    adjustSize();
    double x, y;
    m_viewport->toUI(constrained, x, y);
    // Keep both editors inside the canvas, including on small laptops/Retina.
    const int left = qRound(std::clamp(x + 18, 0.0, double(std::max(0, parentWidget()->width() - width()))));
    const int top = qRound(std::clamp(y + 18, 0.0, double(std::max(0, parentWidget()->height() - height()))));
    move(left, top);
}

void LC_GripInputWidget::focusLength() {
    m_length->setFocus(Qt::OtherFocusReason);
    m_length->selectAll();
}

bool LC_GripInputWidget::eventFilter(QObject* watched, QEvent* event) {
    if (watched != m_length && watched != m_angle) return false;
    if (event->type() == QEvent::ShortcutOverride) {
        const int key = static_cast<QKeyEvent*>(event)->key();
        if (key == Qt::Key_Escape || key == Qt::Key_Tab || key == Qt::Key_Backtab || key == Qt::Key_Return || key == Qt::Key_Enter) {
            event->accept();
            return true;
        }
    }
    if (event->type() != QEvent::KeyPress) return false;
    const int key = static_cast<QKeyEvent*>(event)->key();
    if (key == Qt::Key_Tab || key == Qt::Key_Backtab) {
        auto* next = watched == m_length ? m_angle : m_length;
        next->setFocus(Qt::TabFocusReason);
        next->selectAll();
    } else if (key == Qt::Key_Return || key == Qt::Key_Enter) {
        if (inputValid()) emit accepted();
    } else if (key == Qt::Key_Escape) {
        emit cancelled();
    } else return false;
    event->accept();
    return true;
}
