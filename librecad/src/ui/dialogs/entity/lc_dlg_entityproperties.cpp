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

#include "lc_dlg_entityproperties.h"

#include <QToolButton>

#include "lc_dlg_dimension.h"
#include "lc_hatchpropertieseditingwidget.h"
#include "lc_entitypropertieseditorwidget.h"
#include "lc_propertieseditingwidget_arc.h"
#include "lc_propertieseditingwidget_circle.h"
#include "lc_propertieseditingwidget_ellipse.h"
#include "lc_propertieseditingwidget_hyperbola.h"
#include "lc_propertieseditingwidget_image.h"
#include "lc_propertieseditingwidget_insert.h"
#include "lc_propertieseditingwidget_line.h"
#include "lc_propertieseditingwidget_parabola.h"
#include "lc_propertieseditingwidget_point.h"
#include "lc_propertieseditingwidget_polyline.h"
#include "lc_propertieseditingwidget_spline.h"
#include "lc_propertieseditingwidget_splinepoints.h"
#include "rs_layer.h"
#include "rs_settings.h"
#include "ui_lc_dlg_entityproperties.h"

class LC_Dialog;

LC_DlgEntityProperties::LC_DlgEntityProperties(QWidget* parent, LC_GraphicViewport* viewport, RS_Entity* entity,
                                               const LC_ActionContext::InteractiveInputInfo::InputType inputType, const QString& tag,
                                               const double valueOne, const double valueTwo)
    : LC_Dialog(parent, ""), ui(new Ui::LC_DlgEntityProperties) {
    ui->setupUi(this);

    LC_EntityPropertiesEditorWidget* primaryEditingWidget{nullptr};
    LC_EntityPropertiesEditorWidget* secondaryEditingWidget{nullptr};

    QString dlgName;
    QString windowTitle;
    const RS2::EntityType entityType = entity->rtti();

    prepareTypeSpecificUI(primaryEditingWidget, secondaryEditingWidget, dlgName, windowTitle, entityType);

    if (primaryEditingWidget != nullptr) {
        primaryEditingWidget->setGraphicViewport(viewport);
        primaryEditingWidget->setEntity(entity);

        const auto placeholder = ui->frmGeomertry->parentWidget()->layout()->replaceWidget(ui->frmGeomertry, primaryEditingWidget);
        delete placeholder;

        primaryEditingWidget->setupInteractiveInputWidgets();

        connect(primaryEditingWidget, &LC_PropertiesEditingWidgetEllipse::interactiveInputRequested, this,
                &LC_DlgEntityProperties::onInteractiveInputRequested);
    }
    else {
        ui->frmGeomertry->setVisible(false);
    }

    if (secondaryEditingWidget != nullptr) {
        secondaryEditingWidget->setGraphicViewport(viewport);
        secondaryEditingWidget->setEntity(entity);
        const auto placeholder = ui->frmBottom->parentWidget()->layout()->replaceWidget(ui->frmBottom, secondaryEditingWidget);
        delete placeholder;

        secondaryEditingWidget->setupInteractiveInputWidgets();
        if (inputType != LC_ActionContext::InteractiveInputInfo::NOTNEEDED) {
            if (primaryEditingWidget != nullptr) {
                primaryEditingWidget->interactiveInputUpdate(inputType, tag, valueOne, valueTwo);
            }
        }
        connect(secondaryEditingWidget, &LC_PropertiesEditingWidgetEllipse::interactiveInputRequested, this,
                &LC_DlgEntityProperties::onInteractiveInputRequested);
    }
    else {
        ui->frmBottom->setVisible(false);
        ui->frmBottom->parentWidget()->layout()->removeWidget(ui->frmBottom);
        delete ui->frmBottom;
    }

    setupEntityLayerAndAttributesUI(entity);

    if (primaryEditingWidget != nullptr) {
        if (inputType != LC_ActionContext::InteractiveInputInfo::NOTNEEDED) {
            primaryEditingWidget->interactiveInputUpdate(inputType, tag, valueOne, valueTwo);
        }
    }
    if (secondaryEditingWidget != nullptr) {
        if (inputType != LC_ActionContext::InteractiveInputInfo::NOTNEEDED) {
            secondaryEditingWidget->interactiveInputUpdate(inputType, tag, valueOne, valueTwo);
        }
    }

    setWindowTitle(windowTitle);
    setDialogName(dlgName);

    const bool autoRaiseButtons = LC_GET_ONE_BOOL("Widgets", "DockWidgetsFlatIcons", true);
    QList<QToolButton*> list = findChildren<QToolButton*>();
    for (const auto button : std::as_const(list)) {
        button->setAutoRaise(autoRaiseButtons);
    }
}

LC_DlgEntityProperties::~LC_DlgEntityProperties() {
    delete ui;
}

void LC_DlgEntityProperties::onInteractiveInputRequested(const LC_ActionContext::InteractiveInputInfo::InputType inputType,
                                                         const QString& tag) {
    m_interactiveInputRequested = inputType;
    m_inputTag = tag;
    accept();
}

void LC_DlgEntityProperties::setupEntityLayerAndAttributesUI(RS_Entity* entity) {
    m_entity = entity;
    RS_Graphic* graphic = m_entity->getGraphic();
    if (graphic != nullptr) {
        ui->cbLayer->init(*graphic->getLayerList(), false, false);
    }
    RS_Layer* lay = m_entity->getLayer(false);
    if (lay != nullptr) {
        ui->cbLayer->setLayer(*lay);
    }

    ui->wPen->setPen(m_entity, lay, tr("Pen"));

    connect(ui->cbLayer, &QG_LayerBox::layerChanged, this, &LC_DlgEntityProperties::onLayerChanged);
    connect(ui->wPen, &QG_WidgetPen::penChanged, this, &LC_DlgEntityProperties::onPenChanged);

    if (LC_GET_ONE_BOOL("Appearance", "ShowEntityIDs", false)) {
        ui->lId->setText(QString("ID: %1").arg(entity->getId()));
    }
    else {
        ui->lId->setVisible(false);
    }
}

void LC_DlgEntityProperties::onLayerChanged(RS_Layer* layer) const {
    m_entity->setLayer(layer);
}

void LC_DlgEntityProperties::onPenChanged() const {
    m_entity->setPen(ui->wPen->getPen());
}

void LC_DlgEntityProperties::prepareTypeSpecificUI(LC_EntityPropertiesEditorWidget*& primaryWidget,
                                                   [[maybe_unused]] LC_EntityPropertiesEditorWidget*& secondaryWidget, QString& dlgName,
                                                   QString& windowTitle, const RS2::EntityType entityType) {
    switch (entityType) {
        case RS2::EntityPoint: {
            primaryWidget = new LC_PropertiesEditingWidgetPoint(this);
            windowTitle = tr("Point Properties");
            dlgName = "DlgPointProperties";
            break;
        }
        case RS2::EntityLine: {
            primaryWidget = new LC_PropertiesEditingWidgetLine(this);
            windowTitle = tr("Line Properties");
            dlgName = "DlgLineProperties";
            break;
        }
        case RS2::EntityArc: {
            primaryWidget = new LC_PropertiesEditingWidgetArc(this);
            windowTitle = tr("Arc Properties");
            dlgName = "DlgArcProperties";
            break;
        }
        case RS2::EntityCircle: {
            primaryWidget = new LC_PropertiesEditingWidgetCircle(this);
            windowTitle = tr("Circle Properties");
            dlgName = "DlgCircleProperties";
            break;
        }
        case RS2::EntityEllipse: {
            primaryWidget = new LC_PropertiesEditingWidgetEllipse(this);
            windowTitle = tr("Ellipse Properties");
            dlgName = "DlgEllipseProperties";
            break;
        }
        case RS2::EntityHyperbola: {
          primaryWidget = new LC_PropertiesEditingWidgetHyperbola(this);
          windowTitle = tr("Hyperbola Properties");
          dlgName = "DlgHyperbolaProperties";
          break;
        }
        case RS2::EntityPolyline: {
            primaryWidget = new LC_PropertiesEditingWidgetPolyline(this);
            windowTitle = tr("Polyline Properties");
            dlgName = "DlgPolylineProperties";
            break;
        }
        case RS2::EntityParabola: {
            primaryWidget = new LC_PropertiesEditingWidgetParabola(this);
            windowTitle = tr("Parabola Properties");
            dlgName = "DlgParabolaProperties";
            break;
        }
        case RS2::EntitySpline: {
            primaryWidget = new LC_PropertiesEditingWidgetSpline(this);
            windowTitle = tr("Spline Properties");
            dlgName = "DlgSplineProperties";
            break;
        }
        case RS2::EntitySplinePoints: {
            primaryWidget = new LC_PropertiesEditingWidgetSplinePoints(this);
            windowTitle = tr("Spline Points Properties");
            dlgName = "DlgSplinePointsProperties";
            break;
        }
        case RS2::EntityImage: {
            primaryWidget = new LC_PropertiesEditingWidgetImage(this);
            windowTitle = tr("Image Properties");
            dlgName = "DlgImageProperties";
            break;
        }
        case RS2::EntityInsert: {
            primaryWidget = new LC_PropertiesEditingWidgetInsert(this);
            windowTitle = tr("Insert Properties");
            dlgName = "DlgInsertProperties";
            break;
        }
        case RS2::EntityText: {
          primaryWidget = new LC_TextPropertiesEditingWidget(this);
          windowTitle = tr("Text Properties");
          dlgName = "DlgTextProperties";
          break;
        }
        case RS2::EntityMText: {
          primaryWidget = new LC_MTextPropertiesEditingWidget(this);
          windowTitle = tr("MText Properties");
          dlgName = "DlgMTextProperties";
          break;
        }
        case RS2::EntityHatch: {
            primaryWidget = new LC_HatchPropertiesEditingWidget(this);
            windowTitle = tr("Hatch Properties");
            dlgName = "DlgHatchProperties";
            break;
        }
        default:
            break;
    }
}
