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

#include "lc_propertiesprovider_text.h"

#include "lc_property_double_interactivepick_view.h"
#include "rs_text.h"

void LC_PropertiesProviderText::doCreateEntitySpecificProperties(LC_PropertyContainer* container, const QList<RS_Entity*>& list) {
    const auto contGeometry = createGeometrySection(container);

    addVector<RS_Text>({"insert", tr("Insertion Point"), tr("Point of text insertion")}, [](const RS_Text* e) -> RS_Vector {
                           return e->getInsertionPoint();
                       }, [](const RS_Vector& v, RS_Text* e) -> void {
                           e->setInsertionPoint(v);
                       }, list, contGeometry);

    addVector<RS_Text>({"secondPoint", tr("Second Point"), tr("Second point of text")}, [](const RS_Text* e) -> RS_Vector {
                           return e->getSecondPoint();
                       },/* [](RS_Vector& v, RS_Text* e) -> void {
                       e->setSecondPoint(v);
                   }*/ nullptr, list, contGeometry);

    addWCSAngle<RS_Text>({"angle", tr("Angle"), tr("Text rotation angle")}, [](const RS_Text* e) -> double {
                             return e->getAngle();
                         }, [](const double& v, RS_Text* l) -> void {
                             l->setAngle(v);
                         }, list, contGeometry);

    static LC_EnumDescriptor halignEnumDescriptor = {
        "halign",
        {
            {RS_TextData::HAlign::HALeft, tr("Left")},
            {RS_TextData::HAlign::HACenter, tr("Center")},
            {RS_TextData::HAlign::HARight, tr("Right")},
            {RS_TextData::HAlign::HAAligned, tr("Aligned")},
            {RS_TextData::HAlign::HAMiddle, tr("Middle")},
            {RS_TextData::HAlign::HAFit, tr("Fit")}
        }
    };

    static LC_EnumDescriptor valignEnumDescriptor = {
        "valign",
        {
            {RS_TextData::VAlign::VABaseline, tr("Baseline")},
            {RS_TextData::VAlign::VABottom, tr("Bottom")},
            {RS_TextData::VAlign::VAMiddle, tr("Middle")},
            {RS_TextData::VAlign::VATop, tr("Top")}
        }
    };

    static LC_EnumDescriptor generationDescriptor = {
        "generation",
        {
            {RS_TextData::TextGeneration::None, tr("None")},
            {RS_TextData::TextGeneration::Backward, tr("Backward")},
            {RS_TextData::TextGeneration::UpsideDown, tr("Upside-down")}
        }
    };

    const auto contText = createTextContainer(container);

    addString<RS_Text>({"content", tr("Content"), tr("Content text of MText")}, [](RS_Text* e) -> QString {
                           return e->getText();
                       }, [](const QString& v, RS_Text* e) -> void {
                           e->setText(v);
                       }, list, contText, false);

    addEnum<RS_Text>({"halign", tr("Horizontal Align"), tr("Horizontal align for text")}, &halignEnumDescriptor,
                     [](const RS_Text* e) -> LC_PropertyEnumValueType {
                         return e->getHAlign();
                     }, [](LC_PropertyEnumValueType& v, RS_Text* e) -> void {
                         const auto halign = static_cast<RS_TextData::HAlign>(v);
                         e->setHAlign(halign);
                     }, list, contText);

    addEnum<RS_Text>({"valign", tr("Vertical Align"), tr("Vertical align for text")}, &valignEnumDescriptor,
                     [](const RS_Text* e) -> LC_PropertyEnumValueType {
                         return e->getVAlign();
                     }, [](LC_PropertyEnumValueType& v, RS_Text* e) -> void {
                         const auto valign = static_cast<RS_TextData::VAlign>(v);
                         e->setVAlign(valign);
                     }, list, contText);

    // fixme - add support of text style instead of font name
    addStringFont<RS_Text>({"font", tr("Font"), tr("Font of the text")}, [](RS_Text* e) -> QString {
                               return e->getStyle();
                           }, [](const QString& v, RS_Text* e) -> void {
                               return e->setStyle(v);
                           }, list, contText);

    addLinearDistance<RS_Text>({"width", tr("Width Factor"), tr("Width factor of the text")}, [](const RS_Text* e) -> double {
                                   return e->getWidthRel();
                               }, [](const double& v, RS_Text* l) -> void {
                                   l->setWidthRel(v);
                               }, list, contText, [](LC_PropertyViewDescriptor* desc)-> void {
                                   desc->attributes[LC_PropertyDoubleInteractivePickView::ATTR_POSITIVIE_VALUES_ONLY] = true;
                               });

    addLinearDistance<RS_Text>({"height", tr("Height"), tr("Height of the text")}, [](const RS_Text* e) -> double {
                                   return e->getHeight();
                               }, [](const double& v, RS_Text* l) -> void {
                                   l->setHeight(v);
                               }, list, contText);

    addEnum<RS_Text>({"generation", tr("Generation"), tr("Text generation method")}, &generationDescriptor,
                     [](const RS_Text* e) -> LC_PropertyEnumValueType {
                         return e->getTextGeneration();
                     }, [](LC_PropertyEnumValueType& v, RS_Text* e) -> void {
                         const auto textgen = static_cast<RS_TextData::TextGeneration>(v);
                         e->setTextGeneration(textgen);
                     }, list, contText);
}

void LC_PropertiesProviderText::doCreateSingleEntityCommands(LC_PropertyContainer* cont, RS_Entity* entity) {
    const auto text = static_cast<RS_Text*>(entity);

    const std::list<CommandLinkInfo> commands = {
        {
            tr("Explode operations"),
            {RS2::ActionModifyExplodeText, tr("Explode text "), tr("Explodes text into individual letters")},
            {RS2::ActionBlocksExplode, tr("Explode"), tr("Explodes text to individual strokes")}
        },
        {tr("Other text operations"), {RS2::ActionDrawBoundingBox, tr("Bounding box"), tr("Creation of bounding box for text")}}
    };

    createEntityContextCommands<RS_Text>(commands, cont, text, "textCommands");
}

void LC_PropertiesProviderText::fillComputedProperites([[maybe_unused]] LC_PropertyContainer* container,
                                                       [[maybe_unused]] const QList<RS_Entity*>& entitiesList) {
}
