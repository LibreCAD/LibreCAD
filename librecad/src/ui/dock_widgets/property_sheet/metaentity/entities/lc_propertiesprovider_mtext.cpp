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

#include "lc_propertiesprovider_mtext.h"

#include "rs_mtext.h"

void LC_PropertiesProviderMText::doCreateEntitySpecificProperties(LC_PropertyContainer* container, const QList<RS_Entity*>& list) {
    const auto contGeometry = createGeometrySection(container);

    addVector<RS_MText>({"insert", tr("Insertion Point"), tr("Point of text insertion")}, [](const RS_MText* e) -> RS_Vector {
                            return e->getInsertionPoint();
                        }, [](const RS_Vector& v, RS_MText* e) -> void {
                            e->setInsertionPoint(v);
                        }, list, contGeometry);

    addWCSAngle<RS_MText>({"angle", tr("Angle"), tr("Text rotation angle")}, [](const RS_MText* e) -> double {
                              return e->getAngle();
                          }, [](const double& v, RS_MText* l) -> void {
                              l->setAngle(v);
                          }, list, contGeometry);

    const auto contText = createTextContainer(container);

    addString<RS_MText>({"content", tr("Content"), tr("Content text of MText")}, [](const RS_MText* e) -> QString {
                            return e->getText();
                        }, [](const QString& v, RS_MText* e) -> void {
                            e->setText(v);
                        }, list, contText, true);

    // fixme - add support of text style instead of font name
    addStringFont<RS_MText>({"font", tr("Font"), tr("Font of the text")}, [](const RS_MText* e) -> QString {
                                return e->getStyle();
                            }, [](const QString& v, RS_MText* e) -> void {
                                return e->setStyle(v);
                            }, list, contText);

    addLinearDistance<RS_MText>({"width", tr("Width"), tr("Width of the text")}, [](const RS_MText* e) -> double {
                                    return e->getWidth();
                                }, [](const double& v, RS_MText* l) -> void {
                                    l->setWidth(v);
                                }, list, contText);

    addLinearDistance<RS_MText>({"height", tr("Height"), tr("Height of the text")}, [](const RS_MText* e) -> double {
                                    return e->getHeight();
                                }, [](const double& v, RS_MText* l) -> void {
                                    l->setHeight(v);
                                }, list, contText);

    static LC_EnumDescriptor halignEnumDescriptor = {
        "halign",
        {
            {RS_MTextData::HAlign::HALeft, tr("Left")},
            {RS_MTextData::HAlign::HACenter, tr("Center")},
            {RS_MTextData::HAlign::HARight, tr("Right")}
        }
    };

    addEnum<RS_MText>({"halign", tr("Horizontal Align"), tr("Horizontal align for text")}, &halignEnumDescriptor,
                      [](const RS_MText* e) -> LC_PropertyEnumValueType {
                          return e->getHAlign();
                      }, [](LC_PropertyEnumValueType& v, RS_MText* e) -> void {
                          const auto halign = static_cast<RS_MTextData::HAlign>(v);
                          e->setHAlign(halign);
                      }, list, contText);

    static LC_EnumDescriptor valignEnumDescriptor = {
        "valign",
        {
            {RS_MTextData::VAlign::VABottom, tr("Bottom")},
            {RS_MTextData::VAlign::VAMiddle, tr("Middle")},
            {RS_MTextData::VAlign::VATop, tr("Top")}
        }
    };

    addEnum<RS_MText>({"valign", tr("Vertical Align"), tr("Vertical align for text")}, &valignEnumDescriptor,
                      [](const RS_MText* e) -> LC_PropertyEnumValueType {
                          return e->getVAlign();
                      }, [](LC_PropertyEnumValueType& v, RS_MText* e) -> void {
                          const auto valign = static_cast<RS_MTextData::VAlign>(v);
                          e->setVAlign(valign);
                      }, list, contText);

    static LC_EnumDescriptor drawDirectionEnumDescriptor = {
        "drawDirection",
        {
            {RS_MTextData::MTextDrawingDirection::LeftToRight, tr("Left to right")},
            {RS_MTextData::MTextDrawingDirection::RightToLeft, tr("Right to left")},
            {RS_MTextData::MTextDrawingDirection::TopToBottom, tr("Top to bottom")},
            {RS_MTextData::MTextDrawingDirection::ByStyle, tr("By Style")}
        }
    };

    addEnum<RS_MText>({"drawDirection", tr("Direction"), tr("Drawing direction for the text")}, &drawDirectionEnumDescriptor,
                      [](const RS_MText* e) -> LC_PropertyEnumValueType {
                          return e->getDrawingDirection();
                      }, [](LC_PropertyEnumValueType& v, RS_MText* e) -> void {
                          const auto valign = static_cast<RS_MTextData::MTextDrawingDirection>(v);
                          e->setDrawingDirection(valign);
                      }, list, contText);

    static LC_EnumDescriptor linespaceingEnumDescriptor = {
        "linespacingStyle",
        {{RS_MTextData::MTextLineSpacingStyle::AtLeast, tr("At Least")}, {RS_MTextData::MTextLineSpacingStyle::Exact, tr("Exact")}}
    };

    addEnum<RS_MText>({"linespacingStyle", tr("Line spacing style"), tr("Style of linespacing")}, &linespaceingEnumDescriptor,
                      [](const RS_MText* e) -> LC_PropertyEnumValueType {
                          return e->getLineSpacingStyle();
                      }, [](LC_PropertyEnumValueType& v, RS_MText* e) -> void {
                          const auto valign = static_cast<RS_MTextData::MTextLineSpacingStyle>(v);
                          e->setLineSpacingFactor(valign);
                      }, list, contText);

    addLinearDistance<RS_MText>({"linespacingFactor", tr("Linespacing"), tr("Linespacing factor for the text")},
                                [](const RS_MText* e) -> double {
                                    return e->getLineSpacingFactor();
                                }, [](const double& v, RS_MText* l) -> void {
                                    l->setLineSpacingFactor(v);
                                }, list, contText);
}

void LC_PropertiesProviderMText::fillComputedProperites([[maybe_unused]]LC_PropertyContainer* container, [[maybe_unused]]const QList<RS_Entity*>& entitiesList) {
}

void LC_PropertiesProviderMText::doCreateSingleEntityCommands(LC_PropertyContainer* cont, RS_Entity* entity) {
    const auto text = static_cast<RS_MText*>(entity);

    const std::list<CommandLinkInfo> commands = {
        {tr("Explode operations"),
            {RS2::ActionModifyExplodeText, tr("Explode text"), tr("Explodes text into individual letters")},
            {RS2::ActionBlocksExplode, tr("Explode"), tr("Explodes text to individual strokes")}
       },
        {
            tr("Other text operations"),
            {RS2::ActionDrawBoundingBox, tr("Bounding box"), tr("Creation of bounding box for text")}
        }
    };

    createEntityContextCommands<RS_MText>(commands, cont, text, "textCommands");
}
