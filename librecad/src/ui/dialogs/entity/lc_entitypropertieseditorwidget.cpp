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

#include "lc_entitypropertieseditorwidget.h"

#include <QLineEdit>
#include <QToolButton>

#include "lc_pointpickbutton.h"

LC_EntityPropertiesEditorWidget::LC_EntityPropertiesEditorWidget(QWidget* parent):
   LC_EntityPropertiesEditorSupport(parent) {
}

void LC_EntityPropertiesEditorWidget::interactiveInputUpdate(const LC_ActionContext::InteractiveInputInfo::InputType inputType,
    const QString &tag, const double valueOne, const double valueTwo) const {
    QList<QLineEdit*> list = findChildren<QLineEdit*>();
    QPair<QString, QString> vectorCoordinatesUI;
    if (inputType == LC_ActionContext::InteractiveInputInfo::POINT ||
        inputType == LC_ActionContext::InteractiveInputInfo::POINT_X ||
        inputType == LC_ActionContext::InteractiveInputInfo::POINT_Y) {
        const auto point = RS_Vector(valueOne, valueTwo);
        vectorCoordinatesUI = toUIStr(point);
    }

    bool pointComponentSet = false;
    bool lineEditFound = false;
    for (const auto lineEdit : std::as_const(list)) {
        auto propertyInputType = lineEdit->property("_interactiveInputEdit");
        if (propertyInputType.isValid() && !propertyInputType.isNull()) {
            auto propertyTag = lineEdit->property("_interactiveInputTag");
            QString tagValue = propertyTag.toString();
            if (tag == tagValue) {
                switch (inputType) {
                    case LC_ActionContext::InteractiveInputInfo::DISTANCE: {
                        toUIValue(valueOne, lineEdit);
                        emit lineEdit->editingFinished();
                        lineEditFound = true;
                        break;
                    }
                    case LC_ActionContext::InteractiveInputInfo::ANGLE: {
                        // toUIAngleDeg(valueOne, lineEdit);
                        toUIAngleDegRaw(valueOne, lineEdit);
                        emit lineEdit->editingFinished();
                        lineEditFound = true;
                        break;
                    }
                    case LC_ActionContext::InteractiveInputInfo::POINT:
                    case LC_ActionContext::InteractiveInputInfo::POINT_X:
                    case LC_ActionContext::InteractiveInputInfo::POINT_Y:{
                        const int component = lineEdit->property("_interactiveInputComponent").toInt();
                        if (component == LC_ActionContext::InteractiveInputInfo::POINT_X) {
                            lineEdit->setText(vectorCoordinatesUI.first);
                            if (pointComponentSet) {
                                lineEditFound = true;
                            }
                            else {
                                pointComponentSet = true;
                            }
                            emit lineEdit->editingFinished();
                        }
                        else if (component == LC_ActionContext::InteractiveInputInfo::POINT_Y) {
                            lineEdit->setText(vectorCoordinatesUI.second);
                            if (pointComponentSet) {
                                lineEditFound = true;
                            }
                            else {
                                pointComponentSet = true;
                            }
                            emit lineEdit->editingFinished();
                        }
                        break;
                    }
                    default: {
                        break;
                    }
                }
            }
        }
        if (lineEditFound) {
            lineEdit->setFocus();
            break;
        }
    }
}

void LC_EntityPropertiesEditorWidget::onInteractiveInputButtonClicked([[maybe_unused]]bool checked) {
    const auto senderButton = dynamic_cast<QToolButton*>(sender());
    if (senderButton != nullptr) {
        const auto property = senderButton->property ("_interactiveInputButton");
        if (property.isValid()) {
            const auto inputType = static_cast<LC_ActionContext::InteractiveInputInfo::InputType>(property.toInt());
            const auto tagProperty = senderButton->property ("_interactiveInputTag");
            const QString tag = tagProperty.toString();
            emit interactiveInputRequested(inputType, tag);
        }
    }
}

void LC_EntityPropertiesEditorWidget::pickDistanceSetup(QToolButton* button,
                                                                 const QString &tag, QLineEdit* lineEditOne,
                                                                 QLineEdit* lineEditTwo) {
    setupInteractiveInputControls(button, LC_ActionContext::InteractiveInputInfo::DISTANCE, tag, lineEditOne, lineEditTwo);
}

void LC_EntityPropertiesEditorWidget::pickAngleSetup(QToolButton* button,
                                                                 const QString &tag, QLineEdit* lineEditOne,
                                                                 QLineEdit* lineEditTwo) {
    setupInteractiveInputControls(button, LC_ActionContext::InteractiveInputInfo::ANGLE, tag, lineEditOne, lineEditTwo);
}

void LC_EntityPropertiesEditorWidget::pickPointSetup(const LC_PointPickButton* button,
                                                                 const QString &tag, QLineEdit* lineEditOne,
                                                                 QLineEdit* lineEditTwo) {
    setupInteractiveInputControls(button->getButton(), LC_ActionContext::InteractiveInputInfo::POINT, tag, lineEditOne, lineEditTwo);
}



void LC_EntityPropertiesEditorWidget::setupInteractiveInputControls(QToolButton* button, const LC_ActionContext::InteractiveInputInfo::InputType inputType, const QString &tag, QLineEdit* lineEditOne,
                                                                 QLineEdit* lineEditTwo) {

    button->setProperty ("_interactiveInputButton", inputType);
    button->setProperty ("_interactiveInputTag", tag);
    button->connect(button, &QToolButton::clicked, this, &LC_EntityPropertiesEditorWidget::onInteractiveInputButtonClicked);

    lineEditOne->setProperty ("_interactiveInputEdit", inputType);
    lineEditOne->setProperty("_interactiveInputTag", tag);
    lineEditOne->setProperty("_interactiveInputComponent",LC_ActionContext::InteractiveInputInfo::POINT_X);
    if (lineEditTwo != nullptr) {
        lineEditTwo->setProperty("_interactiveInputTag", tag);
        lineEditTwo->setProperty ("_interactiveInputEdit", inputType);
        lineEditTwo->setProperty("_interactiveInputComponent",LC_ActionContext::InteractiveInputInfo::POINT_Y);
    }
}
