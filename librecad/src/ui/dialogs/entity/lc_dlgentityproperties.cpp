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

#include <QToolButton>

#include "lc_dlgentityproperties.h"
#include "lc_arcpropertieseditingwidget.h"
#include "lc_circlepropertieseditingwidget.h"
#include "lc_dlgdimension.h"
#include "lc_ellipsepropertieseditingwidget.h"
#include "lc_entitypropertieseditorwidget.h"
#include "lc_hyperbolapropertieseditingwidget.h"
#include "lc_imagepropertieseditingwidget.h"
#include "lc_insertpropertieseditingwidget.h"
#include "lc_linepropertieseditingwidget.h"
#include "lc_parabolapropertieseditingwidget.h"
#include "lc_pointpropertieseditingwidget.h"
#include "lc_polylinepropertieseditingwidget.h"
#include "lc_splinepointspropertieseditingwidget.h"
#include "lc_splinepropertieseditingwidget.h"
#include "rs_layer.h"
#include "rs_settings.h"
#include "ui_lc_dlgentityproperties.h"

class LC_Dialog;

LC_DlgEntityProperties::LC_DlgEntityProperties(QWidget *parent, LC_GraphicViewport *viewport, RS_Entity* entity,
    LC_ActionContext::InteractiveInputInfo::InputType inputType, const QString &tag, double valueOne, double valueTwo)
    : LC_Dialog(parent, ""), ui(new Ui::LC_DlgEntityProperties){
    ui->setupUi(this);

    LC_EntityPropertiesEditorWidget* primaryEditingWidget{nullptr};
    LC_EntityPropertiesEditorWidget* secondaryEditingWidget{nullptr};

    QString dlgName;
    QString windowTitle;
    RS2::EntityType entityType = entity->rtti();

    prepareTypeSpecificUI(primaryEditingWidget,  secondaryEditingWidget, dlgName, windowTitle, entityType);

    if (primaryEditingWidget != nullptr) {
        primaryEditingWidget->setGraphicViewport(viewport);
        primaryEditingWidget->setEntity(entity);

        auto placeholder = ui->frmGeomertry->parentWidget()->layout()->replaceWidget(ui->frmGeomertry, primaryEditingWidget);
        delete placeholder;

        primaryEditingWidget->setupInteractiveInputWidgets();


        connect(primaryEditingWidget, &LC_EllipsePropertiesEditingWidget::interactiveInputRequested, this, &LC_DlgEntityProperties::onInteractiveInputRequested);
    }
    else {
        ui->frmGeomertry->setVisible(false);
    }

    if (secondaryEditingWidget != nullptr) {
        secondaryEditingWidget->setGraphicViewport(viewport);
        secondaryEditingWidget->setEntity(entity);
        auto placeholder = ui->frmBottom->parentWidget()->layout()->replaceWidget(ui->frmBottom, secondaryEditingWidget);
        delete placeholder;

        secondaryEditingWidget->setupInteractiveInputWidgets();
        if (inputType != LC_ActionContext::InteractiveInputInfo::NOTNEEDED) {
            primaryEditingWidget->interactiveInputUpdate(inputType, tag, valueOne, valueTwo);
        }
        connect(secondaryEditingWidget, &LC_EllipsePropertiesEditingWidget::interactiveInputRequested, this, &LC_DlgEntityProperties::onInteractiveInputRequested);
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


    bool autoRaiseButtons = LC_GET_ONE_BOOL("Widgets", "DockWidgetsFlatIcons", true);
    QList<QToolButton *> list = findChildren<QToolButton *> ();
    for (auto button : list) {
        button->setAutoRaise(autoRaiseButtons);
    }
}

LC_DlgEntityProperties::~LC_DlgEntityProperties(){
    delete ui;
}


void LC_DlgEntityProperties::onInteractiveInputRequested(LC_ActionContext::InteractiveInputInfo::InputType inputType,
    QString tag) {
    m_interactiveInputRequested = inputType;
    m_inputTag = tag;
    accept();
}


void LC_DlgEntityProperties::setupEntityLayerAndAttributesUI(RS_Entity* entity) {
    m_entity = entity;
    RS_Graphic* graphic = m_entity->getGraphic();
    if (graphic) {
        ui->cbLayer->init(*(graphic->getLayerList()), false, false);
    }
    RS_Layer* lay = m_entity->getLayer(false);
    if (lay) {
        ui->cbLayer->setLayer(*lay);
    }

    ui->wPen->setPen(m_entity, lay, tr("Pen"));

    connect(ui->cbLayer, &QG_LayerBox::layerChanged, this, &LC_DlgEntityProperties::onLayerChanged);
    connect(ui->wPen, &QG_WidgetPen::penChanged, this, &LC_DlgEntityProperties::onPenChanged);

    if (LC_GET_ONE_BOOL("Appearance","ShowEntityIDs", false)){
        ui->lId->setText(QString("ID: %1").arg(entity->getId()));
    }
    else{
        ui->lId->setVisible(false);
    }
}


void LC_DlgEntityProperties::onLayerChanged(RS_Layer* layer) {
    m_entity->setLayer(layer);
}

void LC_DlgEntityProperties::onPenChanged() {
    m_entity->setPen(ui->wPen->getPen());
}

void LC_DlgEntityProperties::prepareTypeSpecificUI(LC_EntityPropertiesEditorWidget*& primaryWidget,
                                                   [[maybe_unused]]LC_EntityPropertiesEditorWidget*& secondaryWidget,
                                                   QString& dlgName, QString& windowTitle, RS2::EntityType entityType) {
    switch (entityType) {
        case RS2::EntityPoint: {
            primaryWidget = new LC_PointPropertiesEditingWidget(this);
            windowTitle = tr("Point Properties");
            dlgName = "DlgPointProperties";
            break;
        }
        case RS2::EntityLine: {
            primaryWidget = new LC_LinePropertiesEditingWidget(this);
            windowTitle = tr("Line Properties");
            dlgName = "DlgLineProperties";
            break;
        }
        case RS2::EntityArc: {
            primaryWidget = new LC_ArcPropertiesEditingWidget(this);
            windowTitle = tr("Arc Properties");
            dlgName = "DlgArcProperties";
            break;
        }
        case RS2::EntityCircle: {
            primaryWidget = new LC_CirclePropertiesEditingWidget(this);
            windowTitle = tr("Circle Properties");
            dlgName = "DlgCircleProperties";
            break;
        }
        case RS2::EntityEllipse: {
            primaryWidget = new LC_EllipsePropertiesEditingWidget(this);
            windowTitle = tr("Ellipse Properties");
            dlgName = "DlgEllipseProperties";
            break;
        }
        case RS2::EntityHyperbola: {
          primaryWidget = new LC_HyperbolaPropertiesEditingWidget(this);
          windowTitle = tr("Hyperbola Properties");
          dlgName = "DlgHyperbolaProperties";
          break;
        }
        case RS2::EntityPolyline: {
            primaryWidget = new LC_PolylinePropertiesEditingWidget(this);
            windowTitle = tr("Polyline Properties");
            dlgName = "DlgPolylineProperties";
            break;
        }
        case RS2::EntityParabola: {
            primaryWidget = new LC_ParabolaPropertiesEditingWidget(this);
            windowTitle = tr("Parabola Properties");
            dlgName = "DlgParabolaProperties";
            break;
        }
        case RS2::EntitySpline: {
            primaryWidget = new LC_SplinePropertiesEditingWidget(this);
            windowTitle = tr("Spline Properties");
            dlgName = "DlgSplineProperties";
            break;
        }
        case RS2::EntitySplinePoints: {
            primaryWidget = new LC_SplinePointsPropertiesEditingWidget(this);
            windowTitle = tr("Spline Points Properties");
            dlgName = "DlgSplinePointsProperties";
            break;
        }
        case RS2::EntityImage: {
            primaryWidget = new LC_ImagePropertiesEditingWidget(this);
            windowTitle = tr("Image Properties");
            dlgName = "DlgImageProperties";
            break;
        }
        case RS2::EntityInsert: {
            primaryWidget = new LC_InsertPropertiesEditingWidget(this);
            windowTitle = tr("Insert Properties");
            dlgName = "DlgInsertProperties";
            break;
        }
        case RS2::EntityText:
        case RS2::EntityMText:
        case RS2::EntityHatch: {
            // propertiesEditingWidget = new LC_ArcPropertiesEditingWidget(this);
            windowTitle = tr("Properties?");
            dlgName = "DlgLineProperties";
            break;
        }
        default:
            break;
    }
}
