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

#ifndef LC_MATCHDESCRIPTORDIMBASE_H
#define LC_MATCHDESCRIPTORDIMBASE_H

#include "lc_dlgdimstylemanager.h"
#include "lc_matchdescriptor_base.h"
#include "lc_property.h"
#include "lc_property_double_spinbox_view.h"
#include "rs_dimension.h"

template <typename EntityType>
class LC_DimensionEntityMatchDescriptor : public LC_TypedEntityMatchDescriptor<EntityType> {
public:
    LC_DimensionEntityMatchDescriptor(const QString& name, RS2::EntityType entityType)
        : LC_TypedEntityMatchDescriptor<EntityType>(name, entityType) {
    }

    void addDoubleDS(const QString& name, std::function<double(LC_DimStyle*)> funAccess, const QString& displayName,
                     const QString& description) {
        this->addDouble(name, [funAccess](EntityType* e) {
                auto dimStyle = e->getEffectiveCachedDimStyle();
                double result = funAccess(dimStyle);
                e->clearCachedDimStyle();
                return result;
            }, displayName, description);
    }

    void addStringDS(const QString& name, std::function<QString(LC_DimStyle*)> funAccess, const QString& displayName,
                     const QString& description) {
        this->addString(name, [funAccess](EntityType* e) {
            auto dimStyle = e->getEffectiveCachedDimStyle();
            QString result = funAccess(dimStyle);
            e->clearCachedDimStyle();
            return result;
        }, displayName, description);
    }

    void addBooleanDS(const QString& name, std::function<double(LC_DimStyle*)> funAccess, const QString& displayName,
                      const QString& description) {
        this->addBoolean(name, [funAccess](EntityType* e)-> bool {
            auto dimStyle = e->getEffectiveCachedDimStyle();
            const bool result = funAccess(dimStyle);
            e->clearCachedDimStyle();
            return result;
        }, displayName, description);
    }

    void addIntEnumDS(const QString& name, std::function<int(LC_DimStyle*)> funAccess, const QString& displayName,
                      const QString& description, std::initializer_list<QPair<QString, int>> choicesList) {
        this->addIntChoice(name, [funAccess](EntityType* e) {
            auto dimStyle = e->getEffectiveCachedDimStyle();
            int result = funAccess(dimStyle);
            e->clearCachedDimStyle();
            return result;
        }, displayName, description, choicesList);
    }

    void addLineWeightDS(const QString& name, std::function<RS2::LineWidth(const LC_DimStyle*)> funAccess, const QString& displayName,
                         const QString& description) {
        this->template add<RS2::LineWidth>(name, [funAccess](EntityType* e) {
            const auto dimStyle = e->getEffectiveCachedDimStyle();
            RS2::LineWidth result = funAccess(dimStyle);
            e->clearCachedDimStyle();
            return result;
        }, displayName, description, LC_PropertyMatcherTypes::LINE_WIDTH);
    }

    void addLineTypeDS(const QString& name, std::function<RS2::LineType(LC_DimStyle*)> funAccess, const QString& displayName,
                       const QString& description) {
        this->template add<RS2::LineType>(name, [funAccess](EntityType* e) {
            auto dimStyle = e->getEffectiveCachedDimStyle();
            RS2::LineType result = funAccess(dimStyle);
            e->clearCachedDimStyle();
            return result;
        }, displayName, description, LC_PropertyMatcherTypes::LINE_TYPE);
    }

    void addColorDS(const QString& name, std::function<RS_Color(LC_DimStyle*)> funAccess, const QString& displayName,
                    const QString& description) {
        this->template add<RS_Color>(name, [funAccess](EntityType* e) {
            auto dimStyle = e->getEffectiveCachedDimStyle();
            RS_Color result = funAccess(dimStyle);
            e->clearCachedDimStyle();
            return result;
        }, displayName, description, LC_PropertyMatcherTypes::COLOR);
    }
};

class LC_MatchDescriptorDimBase : public LC_MatchDescriptorBase {
    Q_OBJECT protected:
    template <typename EntityType>
    static void initCommonDimensionAttributes(LC_DimensionEntityMatchDescriptor<EntityType>* entity, LC_ActionContext *actionContext) {
        entity->addStringList("dimStyle", [](EntityType* e) {
            return e->getStyle();
        }, tr("Style"), tr("Style of dimension"),
        [&](QList<std::pair<QString, QVariant>>& values)->void {
            const auto graphic = actionContext->getDocument()->getGraphic();
            if (graphic != nullptr) {
                const auto dimStyleList = graphic->getDimStyleList();
                if (dimStyleList != nullptr) {
                    const auto stylesList = dimStyleList->getStylesList();
                    for (const auto ds: *stylesList) {
                        RS2::EntityType type;
                        QString baseName;
                        LC_DimStyle::parseStyleName(ds->getName(), baseName, type);
                        if (type == RS2::EntityUnknown) {
                            values.push_back(std::pair(baseName, baseName));
                        }
                    }
                }
            }
        });

        entity->addBoolean("hasStyleOverride", [](EntityType* e) {
            return e->getDimStyleOverride() != nullptr;
        }, tr("Has style override"), tr("Whether dimension has dimension style override or not"));

        RS2::EntityType entityType = entity->getEntityType();
        initLinesAndArrowsAttributesDS(entity, entityType);
        intTextAttributesDS(entity, entityType);
        initFitAttributesDS(entity, entityType);
        initPrimaryUnitsAttributesDS(entity, entityType);
        if (entityType != RS2::EntityDimAngular) {
            initAlternateUnitsAttributesDS(entity, entityType);
        }
        initTolerancesAttributesDS(entity, entityType);
    }

    template <typename EntityType>
    static void initLinesAndArrowsAttributesDS(LC_DimensionEntityMatchDescriptor<EntityType>* entity, const RS2::EntityType entityType) {
        /*
        std::vector<LC_DimArrowRegistry::ArrowInfo> arrowTypes;
        LC_DimArrowRegistry::fillDefaultArrowTypes(arrowTypes);
        auto graphic = m_actionContext->getDocument()->getGraphic();
        bool hasCustomBlocks = false;

        QStringList blocksNames;

        auto blocksList = graphic->getBlockList();
        int blocksCount = blocksList->count();
        if (blocksCount > 0) {
            for (RS_Block* block : *blocksList) {
                QString blockName = block->getName();
                blocksNames << blockName;
            }
            blocksNames.sort();
            hasCustomBlocks = true;
        }
        */

        const bool notDimOrdinate = entityType != RS2::EntityDimOrdinate;
        if (notDimOrdinate) {
            /*addStringList<RS_Dimension>({"dimArrow1", tr("Arrow 1"), tr("Specifies type of the first dimension arrowhead (DIMBLK1 variable)")},
                                        [](RS_Dimension* e) -> QString {
                                            LC_DimStyle* ds = e->getEffectiveCachedDimStyle();
                                            auto arrowhead = ds->arrowhead();
                                            auto chars = arrowhead->obtainFirstArrowName();
                                            return chars;*/

            /*
            addStringList<RS_Dimension>({"dimArrow2", tr("Arrow 2"), tr("Specifies type of the second dimension arrowhead (DIMBLK2 variable)")},
                                        [](RS_Dimension* e) -> QString {
                                            LC_DimStyle* ds = e->getEffectiveCachedDimStyle();
                                            auto arrowhead = ds->arrowhead();
                                            auto chars = arrowhead->obtainSecondArrowName();
                                            return chars;
                                        }, */

            entity->addBoolean("dimFlipArrow1", [](const RS_Dimension* e) {
                return e->isFlipArrow1();
            }, tr("Flip Arrow 1"), tr("Indicates whether first arrow of the dimension is flipped"));

            if (entityType != RS2::EntityDimRadial) {
                entity->addBoolean("dimFlipArrow2", [](const RS_Dimension* e) {
                    return e->isFlipArrow2();
                }, tr("Flip Arrow 2"), tr("Indicates whether second arrow of the dimension is flipped"));
            }

            entity->addDoubleDS("dimArrowSize", [](const LC_DimStyle* ds) {
                return ds->arrowhead()->size();
            }, tr("Arrow size"), tr("Specifies size of the dimension arrowhead (DIMASZ variable)"));
        }

        const bool radialOrDiametric = entityType == RS2::EntityDimRadial || entityType == RS2::EntityDimDiametric;
        if (radialOrDiametric) {
            entity->addIntEnumDS("dimCenterMark", [](const LC_DimStyle* ds) {
                return ds->radial()->drawingMode();
            }, tr("Center mark"), tr("Specified type of center mark on dimension (DIMCEN variable)"), {
                {tr("Mark"), LC_DimStyle::Radial::DRAW_CENTERMARKS},
                {tr("Line"), LC_DimStyle::Radial::DRAW_CENTERLINES},
                {tr("Right to left"), LC_DimStyle::Radial::DRAW_NOTHING}
            });

            entity->addDoubleDS("dimCentermarkSize", [](const LC_DimStyle* ds) {
                return ds->radial()->size();
            }, tr("Center mark size"), tr("Specifies size of the center mark on the dimension (DIMCEN variable)"));
        }

        if (notDimOrdinate) {
            entity->addLineWeightDS("dimLineWeight", [](const LC_DimStyle* ds) -> RS2::LineWidth {
                return ds->dimensionLine()->lineWidth();
            }, tr("Dim line lineweight"), tr("Specifies lineweight for dimension lines (DIMLWD variable)"));

            if (!radialOrDiametric) {
                entity->addLineWeightDS("dimExtLineWeight", [](const LC_DimStyle* ds) -> RS2::LineWidth {
                    return ds->extensionLine()->lineWidth();
                }, tr("Ext line lineweight"), tr("Specifies lineweight for extension line (DIMLWE variable)"));
            }

            entity->addBooleanDS("dimShowDim1", [](const LC_DimStyle* ds) -> bool {
                return !ds->dimensionLine()->isSuppressFirst();
            }, tr("Dim line 1"), tr("Sets suppression of first dimension line (DIMSD1 variable)"));

            entity->addBooleanDS("dimShowDim2", [](const LC_DimStyle* ds) -> bool {
                return !ds->dimensionLine()->isSuppressSecond();
            }, tr("Dim line 2"), tr("Sets suppression of second dimension line (DIMSD2 variable)"));

            entity->addColorDS("dimLineColor", [](const LC_DimStyle* ds) -> RS_Color {
                const auto text = ds->dimensionLine();
                return text->color();
            }, tr("Dim line color"), tr("Specifies color of the dimension line (DIMCLRD variable)"));

            entity->addLineTypeDS("dimLineLineType", [](const LC_DimStyle* ds) -> RS2::LineType {
                return ds->dimensionLine()->lineType();
            }, tr("Dim line linetype"), tr("Specifies the linetype of the dimension line (DIMLTYPE variable)"));

            if (entityType != RS2::EntityDimAngular) {
                if (!radialOrDiametric) {
                    entity->addDoubleDS("dimLineExt", [](const LC_DimStyle* ds) -> double {
                        return ds->text()->height();
                    }, tr("Dim line ext"), tr("Specifies amount to extend dimension lines beyond the extension lines (DIMDLE variable)"));
                }
            }
            if (radialOrDiametric) {
                entity->addBooleanDS("dimExt1Show", [](const LC_DimStyle* ds) -> bool {
                    return !ds->extensionLine()->isSuppressFirst();
                }, tr("Ext line"), tr("Sets suppression of extension line (DIMSE1 variable)"));

                entity->addLineWeightDS("dimExtLineWeight", [](const LC_DimStyle* ds) -> RS2::LineWidth {
                    return ds->extensionLine()->lineWidth();
                }, tr("Ext line lineweight"), tr("Specifies lineweight for extension line (DIMLWE variable)"));

                entity->addLineTypeDS("dimExtLineType", [](const LC_DimStyle* ds) -> RS2::LineType {
                    return ds->extensionLine()->lineTypeFirst();
                }, tr("Ext line linetype"), tr("Specifies the linetype of the extension line (DIMLTEX1 variable)"));
            }
            else {
                entity->addLineTypeDS("dimExt1LineType", [](const LC_DimStyle* ds) -> RS2::LineType {
                    return ds->extensionLine()->lineTypeFirst();
                }, tr("Ext line 1 linetype"), tr("Specifies the linetype of the first extension line (DIMLTEX1 variable)"));

                entity->addLineTypeDS("dimExt2LineType", [](const LC_DimStyle* ds) -> RS2::LineType {
                    return ds->extensionLine()->lineTypeSecond();
                }, tr("Ext line 2 linetype"), tr("Specifies the linetype of the second extension line (DIMLTEX2 variable)"));

                entity->addBooleanDS("dimExt1Show", [](const LC_DimStyle* ds) -> bool {
                    return !ds->extensionLine()->isSuppressFirst();
                }, tr("Ext line 1"), tr("Sets suppression of first extension line (DIMSE1 variable)"));

                entity->addBooleanDS("dimExt2Show", [](const LC_DimStyle* ds) -> bool {
                    return !ds->extensionLine()->isSuppressSecond();
                }, tr("Ext line 2"), tr("Sets suppression of second extension line (DIMSE2 variable)"));
            }
        }
        else {
            entity->addLineWeightDS("dimExtLineWeight", [](const LC_DimStyle* ds) -> RS2::LineWidth {
                return ds->extensionLine()->lineWidth();
            }, tr("Ext line lineweight"), tr("Specifies lineweight for extension line (DIMLWE variable)"));

            entity->addLineTypeDS("dimExtLineType", [](const LC_DimStyle* ds) -> RS2::LineType {
                return ds->extensionLine()->lineTypeFirst();
            }, tr("Ext line linetype"), tr("Specifies the linetype for the extension line (DIMLTEX1 variable)"));
        }
        if (!radialOrDiametric) {
            entity->addBooleanDS("dimExtFixed", [](const LC_DimStyle* ds) -> bool {
                return ds->extensionLine()->hasFixedLength();
            }, tr("Ext line fixed"), tr("Sets suppression of extension line fixed length (DIMFXLON variable)"));

            entity->addDoubleDS("dimExtFixedLen", [](const LC_DimStyle* ds) -> double {
                return ds->extensionLine()->fixedLength();
            }, tr("Ext line fixed length"), tr("Sets extension line fixed length (DIMFXL variable)"));
        }

        entity->addColorDS("dimExtLineColor", [](const LC_DimStyle* ds) -> RS_Color {
            const auto line = ds->extensionLine();
            return line->color();
        }, tr("Ext line color"), tr("Specifies color of the extension line (DIMCLRE variable)"));

        if (notDimOrdinate) {
            entity->addDoubleDS("dimExtExtent", [](const LC_DimStyle* ds) -> double {
                return ds->extensionLine()->distanceBeyondDimLine();
            }, tr("Ext line ext"), tr("Specifies amount to extend extension line beyond the dimension line  (DIMEXE variable)"));
        }

        entity->addDoubleDS("dimExtOffset", [](const LC_DimStyle* ds) -> double {
            return ds->extensionLine()->distanceFromOriginPoint();
        }, tr("Ext line offset"), tr("Specifies offset of extension lines from the origin points (DIMEXO variable)"));
    }

    template <typename EntityType>
    static void intTextAttributesDS(LC_DimensionEntityMatchDescriptor<EntityType>* entity, const RS2::EntityType entityType) {
        entity->addColorDS("dimTextFillColor", [](const LC_DimStyle* ds) -> RS_Color {
            const auto text = ds->text();
            return text->explicitBackgroundFillColor();
        }, tr("Fill color"), tr("Specifies the background color of the dimension (DIMTFILL variable)"));

        entity->addColorDS("dimTextColor", [](const LC_DimStyle* ds) -> RS_Color {
            const auto text = ds->text();
            return text->color();
        }, tr("Text color"), tr("Specifies the color of the dimension of the text (DIMCLRT variable)"));

        entity->addDoubleDS("dimTextHeight", [](const LC_DimStyle* ds) -> double {
            return ds->text()->height();
        }, tr("Text height"), tr("Specifies text height of dimension (DIMTXT variable)"));

        entity->addDoubleDS("dimTextOffset", [](const LC_DimStyle* ds) -> double {
                                return ds->dimensionLine()->lineGap();
                            }, tr("Text offset"),
                            tr(
                                "Specifies the distance around dimension text when dimension line breaks for dimension text (DIMGAP variable)"));

        if (entityType != RS2::EntityDimOrdinate) {
            entity->addBooleanDS("dimTextOrientationOutside", [](const LC_DimStyle* ds) -> bool {
                return ds->text()->isAlignedIfOutside();
            }, tr("Text outside align"), tr("Sets positioning of dimension text outside of extension lines (DIMTOH variable)"));
        }

        if ((entityType != RS2::EntityDimOrdinate) && (entityType != RS2::EntityDimRadial) && (entityType != RS2::EntityDimDiametric)) {
            entity->addIntEnumDS("dimTextHor", [](const LC_DimStyle* ds) -> int {
                return ds->text()->horizontalPositioning();
            }, tr("Text pos hor"), tr("Specified horizontal dimension text position (DIMJUST variable)"), {
                {tr("Centered"), LC_DimStyle::Text::ABOVE_AND_CENTERED},
                {tr("First ext line"), LC_DimStyle::Text::NEXT_TO_EXT_ONE},
                {tr("Second ext line"), LC_DimStyle::Text::NEXT_TO_EXT_TWO},
                {tr("Over first ext line"), LC_DimStyle::Text::ABOVE_ALIGN_EXT_ONE},
                {tr("Over second ext line"), LC_DimStyle::Text::ABOVE_ALIGN_EXT_TWO}
            });
        }

        entity->addIntEnumDS("dimTextVert", [](const LC_DimStyle* ds) -> int {
            return ds->text()->verticalPositioning();
        }, tr("Text pos vert"), tr("Specified vertical dimension text position (DIMTAD variable)"), {
            {tr("Centered"), LC_DimStyle::Text::CENTER_BETWEEN_EXT_LINES},
            {tr("Above"), LC_DimStyle::Text::ABOVE_DIM_LINE_EXCEPT_NOT_HORIZONTAL},
            {tr("Outside"), LC_DimStyle::Text::FAREST_SIDE_FROM_DEF_POINTS},
            {tr("JIS"), LC_DimStyle::Text::JIS_POSTION},
            {tr("Below"), LC_DimStyle::Text::BELOW_DIMENSION_LINE}
        });

        // fixme - add support of text styles instead of fonts
        entity->addFontStringList("dimTxtStyle", [](RS_Dimension* e) -> QString {
            const LC_DimStyle* dimStyle = e->getEffectiveCachedDimStyle();
            QString result = dimStyle->text()->style();
            e->clearCachedDimStyle();
            return result;
        }, tr("Text style"), tr("Specifies style of text for dimension (DIMTXSTY variable)"));

        if (entityType != RS2::EntityDimOrdinate) {
            entity->addBooleanDS("dimTextOrientationInside", [](const LC_DimStyle* ds) -> bool {
                return ds->text()->isAlignedIfInside();
            }, tr("Text inside align"), tr("Sets positioning of dimension text inside of extension lines (DIMTIH variable)"));
        }

        entity->addVectorX("startX", [](EntityType* e) {
            return e->getMiddleOfText();
        }, tr("Text position X"), tr("X coordinate for dimension text position"));

        entity->addVectorY("startY", [](EntityType* e) {
            return e->getMiddleOfText();
        }, tr("Text position Y"), tr("Y coordinate for dimension text position"));

        if (entityType == RS2::EntityDimArc) {
            entity->addIntEnumDS("dimTextArcLen", [](const LC_DimStyle* ds) -> int {
                return ds->arc()->arcSymbolPosition();
            }, tr("Arc length symbol"), tr("Specifies placement of the arch length dimension symbol (DIMARCSYM variable)"), {
                {tr("Preceeding dimension text"), LC_DimStyle::Arc::BEFORE},
                {tr("Above dimension text"), LC_DimStyle::Arc::ABOVE},
                {tr("None"), LC_DimStyle::Arc::NONE}
            });
        }

        // fixme - check whether wcs or relative angle should be used
        if (entityType != RS2::EntityDimArc) {
            entity->addAngle("angle", [](EntityType* e) {
                return e->getTextAngle();
            }, tr("Angle"), tr("Text rotation angle"));
        }

        entity->addIntEnumDS("dimTextDir", [](const LC_DimStyle* ds) -> int {
            return ds->text()->readingDirection();
        }, tr("Text view direction"), tr("Specifies the text direction (DIMTXTDIRECTION variable)"), {
            {tr("Left-to-right"), LC_DimStyle::Text::LEFT_TO_RIGHT},
            {tr("Right-to-left"), LC_DimStyle::Text::RIGHT_TO_LEFT}
        });

        /*addReadOnlyString<RS_Dimension>({"dimTextMeasurement", tr("Text measurement"), tr("Specified dimension measurement value")},
                                        [this](RS_Dimension* e)-> QString {
                                            double measurement = e->getMeasurement();
                                            QString value;
                                            if (e->rtti() == RS2::EntityDimAngular) {
                                                value = formatRawAngle(measurement, RS2::AngleFormat::DegreesDecimal);
                                            }
                                            else {
                                                value = formatDouble(measurement);
                                            }
                                            return value;
                                        }, list, cont);*/

        entity->addString("dimTextOverride", [](EntityType* e) -> QString {
            return e->getText();
        }, tr("Text override"), tr("Specified the text string of dimension (overrides Measurement string)"));
    }

    template <typename EntityType>
    static void initFitAttributesDS(LC_DimensionEntityMatchDescriptor<EntityType>* entity, const RS2::EntityType entityType) {
        const bool notOrdinateDimension = entityType != RS2::EntityDimOrdinate;
        if (notOrdinateDimension) {
            entity->addBooleanDS("dimLinOutside", [](const LC_DimStyle* ds) -> bool {
                                     return ds->dimensionLine()->isDrawLineIfArrowsOutside();
                                 }, tr("Dim line forced"),
                                 tr("Force drawing dimension line between extension lines,"
                                     " even if text placed outside of extension lines (DIMTOFL variable)"));

            if (entityType != RS2::EntityDimDiametric && entityType != RS2::EntityDimRadial) {
                entity->addBooleanDS("dimLineInside", [](const LC_DimStyle* ds) -> bool {
                    return !ds->arrowhead()->isSuppressArrows();
                }, tr("Dim line inside"), tr("Force displaying dimension line outside extension lines (DIMSOXD variable)"));
            }
        }

        entity->addDoubleDS("dimScale", [](const LC_DimStyle* ds) -> double {
                                return ds->scaling()->scale();
                            }, tr("Dim scale overall"),
                            tr("Specifies the overall scale factor applied to properties, that "
                                "specify sizes, distances or offsets (DIMSCALE variable)"));

        if (notOrdinateDimension) {
            entity->addIntEnumDS("fit", [](const LC_DimStyle* ds) -> int {
                const LC_DimStyle::Text* text = ds->text();
                const auto policy = text->unsufficientSpacePolicy();

                bool setByDIMATFIT = false;
                int value = 0;
                switch (policy) {
                    case LC_DimStyle::Text::EITHER_TEXT_OR_ARROW: {
                        value = 0;
                        setByDIMATFIT = true;
                        break;
                    }
                    case LC_DimStyle::Text::ARROW_FIRST_THEN_TEXT: {
                        value = 1;
                        setByDIMATFIT = true;
                        break;
                    }
                    case LC_DimStyle::Text::TEXT_FIRST_THEN_ARROW: {
                        value = 2;
                        setByDIMATFIT = true;
                        break;
                    }
                    case LC_DimStyle::Text::OUTSIDE_EXT_LINES: {
                        value = 3;
                        setByDIMATFIT = true;
                        break;
                    }
                }

                if (!setByDIMATFIT) {
                    if (text->extLinesRelativePlacement() == LC_DimStyle::Text::PLACE_ALWAYS_INSIDE) {
                        value = 4;
                    }
                }
                return value;
            }, tr("Fit"), tr(
                "Determines what elements are moved to fit text and arrowheads in space between extension lines (DIMATFIT variable)"), {
                {tr("Best Fit"), 0},
                {tr("Arrows"), 1},
                {tr("Text"), 2},
                {tr("Both"), 3},
                {tr("Always fit"), 4}
            });

            entity->addIntEnumDS("textInside", [](const LC_DimStyle* ds) -> bool {
                return ds->text()->extLinesRelativePlacement();
            }, tr("Text inside"), tr("Sets position of dimension text inside extension lines (DIMTIX variable)"), {
                {tr("Place bettween if has room"), LC_DimStyle::Text::PLACE_BETWEEN_IF_SUFFICIENT_ROOM},
                {tr("Always inside"), LC_DimStyle::Text::PLACE_ALWAYS_INSIDE}
            });
        }

        entity->addIntEnumDS("textMovement", [](const LC_DimStyle* ds) {
                                 const LC_DimStyle::Text* text = ds->text();
                                 auto policy = text->positionMovementPolicy();
                                 return policy;
                             }, tr("Text movement"),
                             tr("Specifies position of the text when it is moved, either manually or automatically (DIMTMOVE variable)"),
                             {
                                 {tr("Keep dim line with text"), LC_DimStyle::Text::DIM_LINE_WITH_TEXT},
                                 {tr("Move text, add leader"), LC_DimStyle::Text::ADDS_LEADER},
                                 {tr("Move text, no leader"), LC_DimStyle::Text::ALLOW_FREE_POSITIONING}
                             });
    }

    template <typename EntityType>
    static void initPrimaryUnitsAttributesDS(LC_DimensionEntityMatchDescriptor<EntityType>* entity, const RS2::EntityType entityType) {
        const bool notAngularDimension = entityType != RS2::EntityDimAngular;

        entity->addIntEnumDS("decimalSeparator", [](const LC_DimStyle* ds) {
                                 const auto linearFormat = ds->linearFormat();
                                 auto separator = linearFormat->decimalFormatSeparatorChar();
                                 return separator;
                             }, tr("Decimal separator"), tr("Specifies the decimal separator for metric dimensions (DIMDSEP variable)"),
                             {{tr("."), 43}, {tr(","), 44}});

        entity->addStringDS("dimPrimaryPrefix", [](const LC_DimStyle* ds) -> QString {
            const auto linearFormat = ds->linearFormat();
            LC_DimStyle::LinearFormat::TextPattern* pattern = linearFormat->getPrimaryPrefixOrSuffix();
            return pattern->getPrefix();
        }, tr("Dim prefix"), tr("Specifies the text prefix for the dimensions (DIMPOST variable)"));

        entity->addStringDS("dimPrimarySuffix", [](const LC_DimStyle* ds) -> QString {
            const auto linearFormat = ds->linearFormat();
            LC_DimStyle::LinearFormat::TextPattern* pattern = linearFormat->getPrimaryPrefixOrSuffix();
            return pattern->getSuffix();
        }, tr("Dim suffix"), tr("Specifies the text suffix for the dimensions (DIMPOST variable)"));

        if (notAngularDimension) {
            entity->addDoubleDS("dimPrimaryRoundoff", [](const LC_DimStyle* ds) -> double {
                const auto roundoff = ds->roundOff();
                return roundoff->roundTo();
            }, tr("Dim roundoff"), tr("Specifies the distance rounding value (DIMRND variable)"));

            entity->addDoubleDS("dimPrimaryScalelinear", [](const LC_DimStyle* ds) -> double {
                const auto scaling = ds->scaling();
                return scaling->linearFactor();
            }, tr("Dim scale linear"), tr("Specifies global scale factor for linear dimensions (DIMLFAC variable)"));

            /*
      // fimxe - RESTORE!
            if (entityType != RS2::EntityDimDiametric && entityType != RS2::EntityDimRadial) {
                entity->addDoubleDS("dimPrimarySubUnitsScale", [](LC_DimStyle* ds) -> double {
                    auto suppression = ds->zerosSuppression();
                    return suppression->roundTo(); // fixme - dims - where from it's obtained?
                }, tr("Dim sub-units scale"), tr("Specifies sub-units scale factor for all applicable linear dimensions"));
            }
    */

            entity->addIntEnumDS("dimPrimaryLinearUnits", [](const LC_DimStyle* ds) -> int {
                const auto linearFormat = ds->linearFormat();
                const auto format = linearFormat->format();
                return format;
            }, tr("Dim units"), tr("Specifies units format for linear dimensions (DIMLUNIT variable)"), {
                {tr("Scientific"), RS2::LinearFormat::Scientific},
                {tr("Decimal"), RS2::LinearFormat::Decimal},
                {tr("Engineering"), RS2::LinearFormat::Engineering},
                {tr("Architectural"), RS2::LinearFormat::Architectural},
                {tr("Fractional"), RS2::LinearFormat::Fractional},
                {tr("Architectural (metric)"), RS2::LinearFormat::ArchitecturalMetric}
            });
        }

        entity->addBooleanDS("dimPrimarySuppressLeading", [](const LC_DimStyle* ds) -> bool {
            return ds->zerosSuppression()->isLinearSuppress(LC_DimStyle::ZerosSuppression::LEADING_IN_DECIMAL);
        }, tr("Suppress leading zeros"), tr("Sets suppression of leading zeros for dimensions (DIMZIN variable)"));

        entity->addBooleanDS("dimPrimarySuppressTrailing", [](const LC_DimStyle* ds) -> bool {
            return ds->zerosSuppression()->isLinearSuppress(LC_DimStyle::ZerosSuppression::TRAILING_IN_DECIMAL);
        }, tr("Suppress trailing zeros"), tr("Sets suppression of trailing zeros for dimensions (DIMZIN variable)"));

        if (notAngularDimension) {
            entity->addBooleanDS("dimPrimarySuppressZeroFeet", [](const LC_DimStyle* ds) -> bool {
                const auto zerosSuppression = ds->zerosSuppression();
                bool feetSuppress = false;
                if (zerosSuppression->isLinearSuppress(LC_DimStyle::ZerosSuppression::INCLUDE_ZERO_FEET_AND_ZERO_INCHES)) {
                    if (zerosSuppression->isLinearSuppress(LC_DimStyle::ZerosSuppression::INCLUDE_ZERO_FEET_AND_SUPPRESS_ZERO_INCHES)) {
                        feetSuppress = true;
                    }
                }
                else {
                    if (!zerosSuppression->isLinearSuppress(LC_DimStyle::ZerosSuppression::INCLUDE_ZERO_FEET_AND_SUPPRESS_ZERO_INCHES)) {
                        feetSuppress = true;
                    }
                }
                return feetSuppress;
            }, tr("Suppress zero feet"), tr("Sets suppression of zero feet in dimension (DIMZIN variable)"));

            entity->addBooleanDS("dimPrimarySuppressZeroInches", [](const LC_DimStyle* ds) -> bool {
                const auto zerosSuppression = ds->zerosSuppression();
                const bool inchesSuppress = zerosSuppression->isLinearSuppress(LC_DimStyle::ZerosSuppression::INCLUDE_ZERO_FEET_AND_ZERO_INCHES)
                    && !zerosSuppression->isLinearSuppress(LC_DimStyle::ZerosSuppression::INCLUDE_ZERO_FEET_AND_SUPPRESS_ZERO_INCHES);
                return inchesSuppress;
            }, tr("Suppress zero inches"), tr("Sets suppression of zero inches for dimension (DIMZIN variable)"));

            entity->addIntEnumDS("dimPrimaryDecimalPlaces", [](const LC_DimStyle* ds) -> int {
                const auto linearFormat = ds->linearFormat();
                const auto format = linearFormat->decimalPlaces();
                return format;
            }, tr("Precision"), tr("Specifies precision for primary units dimensions (DIMDEC variable)"), {
                {tr("1.00"), 1},
                {tr("0.1"), 2},
                {tr("0.01"), 3},
                {tr("0.001"), 4},
                {tr("0.0001"), 5},
                {tr("0.00001"), 6},
                {tr("0.000001"), 7},
                {tr("0.0000001"), 8},
                {tr("0.00000001"), 9}
            });
        }
        else {
            entity->addIntEnumDS("dimPrimaryAngularPlaces", [](const LC_DimStyle* ds) -> int {
                                     const auto linearFormat = ds->angularFormat();
                                     const auto format = linearFormat->decimalPlaces();
                                     return format;
                                 }, tr("Angle precision"),
                                 tr("Specifies number of precision decimal places displayed for angular dimension text (DIMADEC variable)"),
                                 {
                                     {tr("1.00"), 1},
                                     {tr("0.10"), 2},
                                     {tr("0.01"), 3},
                                     {tr("0.001"), 4},
                                     {tr("0.0001"), 5},
                                     {tr("0.00001"), 6},
                                     {tr("0.000001"), 7},
                                     {tr("0.0000001"), 8},
                                     {tr("0.00000001"), 9}
                                 });

            entity->addIntEnumDS("dimPrimaryAngularFormat", [](const LC_DimStyle* ds) -> int {
                const auto angularFormat = ds->angularFormat();
                const auto format = angularFormat->format();
                return format;
            }, tr("Angle format"), tr("Specifies the angle format (DIMAUNIT variable)"), {
                {tr("Decimal Degrees"), RS2::AngleFormat::DegreesDecimal},
                {tr("Deg/min/sec"), RS2::AngleFormat::DegreesMinutesSeconds},
                {tr("Gradians"), RS2::AngleFormat::Gradians},
                {tr("Radians"), RS2::AngleFormat::Radians},
                {tr("Surveyor's units"), RS2::AngleFormat::Surveyors}
            });
        }
    }

    template <typename EntityType>
    static void initAlternateUnitsAttributesDS(LC_DimensionEntityMatchDescriptor<EntityType>* entity, const RS2::EntityType entityType) {
        entity->addBooleanDS("dimAltEnabled", [](const LC_DimStyle* ds) -> bool {
            return ds->hasAltUnits();
        }, tr("Alt enabled"), tr("Sets units format for alternate units dimensions On or Off except angular (DIMALT variable)"));

        entity->addIntEnumDS("dimAltLinearUnits", [](const LC_DimStyle* ds) {
            const auto linearFormat = ds->linearFormat();
            auto format = linearFormat->altFormat();
            return format;
        }, tr("Alt format"), tr("Specifies units format for alternate units dimensions except angular (DIMALTU variable)"), {
            {tr("Scientific"), RS2::LinearFormat::Scientific},
            {tr("Decimal"), RS2::LinearFormat::Decimal},
            {tr("Engineering"), RS2::LinearFormat::Engineering},
            {tr("Architectural"), RS2::LinearFormat::Architectural},
            {tr("Fractional"), RS2::LinearFormat::Fractional},
            {tr("Architectural (metric)"), RS2::LinearFormat::ArchitecturalMetric}
        });

        entity->addIntEnumDS("dimAltDecimalPlaces", [](const LC_DimStyle* ds) -> int {
            const auto linearFormat = ds->linearFormat();
            const auto format = linearFormat->altDecimalPlaces();
            return format;
        }, tr("Alt precision"), tr("Specifies precision for alternate units dimensions (DIMALTD variable)"), {
            {tr("1.00"), 1},
            {tr("0.1"), 2},
            {tr("0.01"), 3},
            {tr("0.001"), 4},
            {tr("0.0001"), 5},
            {tr("0.00001"), 6},
            {tr("0.000001"), 7},
            {tr("0.0000001"), 8},
            {tr("0.00000001"), 9}
        });

        entity->addDoubleDS("dimAltRoundoff", [](const LC_DimStyle* ds) -> double {
            const auto roundoff = ds->roundOff();
            return roundoff->altRoundTo();
        }, tr("Alt round"), tr("Specifies distances rounding value for alternate units (DIMALTRND variable)"));

        entity->addDoubleDS("dimAltScaleLinear", [](const LC_DimStyle* ds) -> double {
            const auto linearFormat = ds->linearFormat();
            return linearFormat->altUnitsMultiplier();
        }, tr("Alt scale factor"), tr("Specifies scale factor for alternative units (DIMALTF variable)"));

        entity->addBooleanDS("dimAltSuppressLeading", [](const LC_DimStyle* ds) -> bool {
            return ds->zerosSuppression()->isAltLinearSuppress(LC_DimStyle::ZerosSuppression::LEADING_IN_DECIMAL);
        }, tr("Alt suppress leading zeros"), tr("Sets suppression of leading zeros for alternate units in dimension (DIMALTZ variable)"));

        entity->addBooleanDS("dimAltSuppressTrailing", [](const LC_DimStyle* ds) -> bool {
            return ds->zerosSuppression()->isAltLinearSuppress(LC_DimStyle::ZerosSuppression::TRAILING_IN_DECIMAL);
        }, tr("Alt suppress trailing zeros"), tr("Sets suppression of trailing zeros for alternate units in dimension (DIMALTZ variable)"));

        entity->addBooleanDS("dimAltSuppressZeroFeet", [](const LC_DimStyle* ds) -> bool {
            const auto zerosSuppression = ds->zerosSuppression();
            bool feetSuppress = false;
            if (zerosSuppression->isAltLinearSuppress(LC_DimStyle::ZerosSuppression::INCLUDE_ZERO_FEET_AND_ZERO_INCHES)) {
                if (zerosSuppression->isAltLinearSuppress(LC_DimStyle::ZerosSuppression::INCLUDE_ZERO_FEET_AND_SUPPRESS_ZERO_INCHES)) {
                    feetSuppress = true;
                }
            }
            else {
                if (!zerosSuppression->isAltLinearSuppress(LC_DimStyle::ZerosSuppression::INCLUDE_ZERO_FEET_AND_SUPPRESS_ZERO_INCHES)) {
                    feetSuppress = true;
                }
            }
            return feetSuppress;
        }, tr("Alt suppress zero feet"), tr("Sets suppression of zero feet for alternate units in dimension (DIMALTZ variable)"));

        entity->addBooleanDS("dimAltSuppressZeroInches", [](const LC_DimStyle* ds) -> bool {
            const auto zerosSuppression = ds->zerosSuppression();
            const bool inchesSuppress = zerosSuppression->isAltLinearSuppress(LC_DimStyle::ZerosSuppression::INCLUDE_ZERO_FEET_AND_ZERO_INCHES) &&
                !zerosSuppression->isAltLinearSuppress(LC_DimStyle::ZerosSuppression::INCLUDE_ZERO_FEET_AND_SUPPRESS_ZERO_INCHES);
            return inchesSuppress;
        }, tr("Alt suppress zero inches"), tr("Sets suppression of zero inches for alternate units in dimension (DIMALTZ variable)"));

        entity->addStringDS("dimAltPrefix", [](const LC_DimStyle* ds) -> QString {
            const auto linearFormat = ds->linearFormat();
            LC_DimStyle::LinearFormat::TextPattern* pattern = linearFormat->getAlternativePrefixOrSuffix();
            return pattern->getPrefix();
        }, tr("Alt prefix"), tr("Specifies text prefix to alternate dimensions except angular (DIMAPOST variable)"));

        entity->addStringDS("dimAltSuffix", [](const LC_DimStyle* ds) -> QString {
            const auto linearFormat = ds->linearFormat();
            LC_DimStyle::LinearFormat::TextPattern* pattern = linearFormat->getAlternativePrefixOrSuffix();
            return pattern->getSuffix();
        }, tr("Alt suffix"), tr("Specifies text suffix to alternate dimensions except angular (DIMAPOST variable)"));

        if (entityType != RS2::EntityDimDiametric && entityType != RS2::EntityDimRadial) {
            // fixme - complete implementation! Define where it's stored
            // addDouble<RS_Dimension>({
            //                             "dimAltSubUnitsSuffix",
            //                             tr("Alt sub-units suffix"),
            //                             tr("Specifies sub-units suffix applicable for linear dimensions")
            //                         }, [](RS_Dimension* e) -> double {
            //                             // same logic as in LC_DlgDimStyleManager::fillPrimaryUnitTab
            //                             LC_DimStyle* dimStyle = e->getEffectiveCachedDimStyle();
            //                             auto suppression = dimStyle->zerosSuppression();
            //                             return /*suppression->roundTo();*/ 0.0; // fixme - dims - where from it's obtained?
            //                         }, nullptr/*[](double& v, [[maybe_unused]] LC_PropertyChangeReason reason, RS_Dimension* e) -> void {
            //                         LC_DimStyle* dimStyle = e->getEffectiveCachedDimStyle();
            //                         auto roundoff = dimStyle->roundOff();
            //                         roundoff->setRoundToValue(v);
            //                         e->setDimStyleOverride(dimStyle);
            //                     }*/, list, cont, funPositiveDoubleSpin);
        }
    }

    template <typename EntityType>
    static void initTolerancesAttributesDS(LC_DimensionEntityMatchDescriptor<EntityType>* entity, const RS2::EntityType entityType) {
        entity->addIntEnumDS("dimTolAlign", [](const LC_DimStyle* ds) {
            return ds->latteralTolerance()->adjustment();
        }, tr("Tolerance alignment"), tr("Specifies alignment for stacked numbers (DIMTALN variable)"), {
            {tr("Operational symbols"), LC_DimStyle::LatteralTolerance::ALIGN_OPERATIONAL_SYMBOLS},
            {tr("Decimal separators"), LC_DimStyle::LatteralTolerance::ALIGN_DECIMAL_SEPARATORS}
        });

        entity->addIntEnumDS("dimTolDisplay", [](const LC_DimStyle* ds) -> int {
            const auto tolerance = ds->latteralTolerance();
            bool enable, showVerticalPosition, showLowerLimit, showUpperLimit;
            const int tolMethod = LC_DlgDimStyleManager::computeToleranceMethod(ds, tolerance, enable, showVerticalPosition, showLowerLimit,
                                                                          showUpperLimit);
            return tolMethod;
        }, tr("Tolerance display"), tr("Specifies display mode of dimension tolerances to dimension text (DIMTOL variable)"), {
            {tr("None"), 0},
            {tr("Symmetrical"), 1},
            {tr("Deviation"), 2},
            {tr("Limits"), 3},
            {tr("Basic"), 4}
        });

        entity->addDoubleDS("dimTolLimitLower", [](const LC_DimStyle* ds) -> double {
                                return ds->latteralTolerance()->lowerToleranceLimit();
                            }, tr("Tolerance limit lower"),
                            tr(
                                "Specifies the minimal (or lower) tolerance limit for dimension text when DIMTOL or DIMLIM is on (DIMTM variable)"));

        entity->addDoubleDS("dimTolLimitUpper", [](const LC_DimStyle* ds) -> double {
                                return ds->latteralTolerance()->upperToleranceLimit();
                            }, tr("Tolerance limit upper"),
                            tr(
                                "Specifies the maximum (or upper) tolerance limit for dimension text when DIMTOL or DIMLIM is on (DIMTP variable)"));

        entity->addIntEnumDS("dimTolPosVert", [](const LC_DimStyle* ds) -> int {
                                 const auto tolerance = ds->latteralTolerance();
                                 return tolerance->verticalJustification();
                             }, tr("Tolerance pos vert"),
                             tr(
                                 "Specifies vertical justification for tolerance values relative to nominal dimension text (DIMTOLJ variable)"),
                             {
                                 {tr("Bottom"), LC_DimStyle::LatteralTolerance::VerticalJustificationToDimText::BOTTOM},
                                 {tr("Middle"), LC_DimStyle::LatteralTolerance::VerticalJustificationToDimText::MIDDLE},
                                 {tr("Top"), LC_DimStyle::LatteralTolerance::VerticalJustificationToDimText::TOP}
                             });

        const bool notAngularDimension = entityType != RS2::EntityDimAngular;
        if (notAngularDimension) {
            entity->addIntEnumDS("dimTolPrecision", [](const LC_DimStyle* ds) -> int {
                const auto tol = ds->latteralTolerance();
                const auto places = tol->decimalPlaces();
                return places;
            }, tr("Tolerance precision"), tr("Specifies number of decimal places for tolerance values of a dimension (DIMTDEC variable)"), {
                {tr("1.00"), 1},
                {tr("0.1"), 2},
                {tr("0.01"), 3},
                {tr("0.001"), 4},
                {tr("0.0001"), 5},
                {tr("0.00001"), 6},
                {tr("0.000001"), 7},
                {tr("0.0000001"), 8},
                {tr("0.00000001"), 9}
            });
        }
        else {
            entity->addIntEnumDS("dimTolAngleDecimalPlaces", [](const LC_DimStyle* ds) -> int {
                const auto tol = ds->latteralTolerance();
                const auto format = tol->decimalPlaces();
                return format;
            }, tr("Tolerance precision"), tr("Specifies number of decimal places for tolerance values of a dimension (DIMTDEC variable)"), {
                {tr("1.00"), 1},
                {tr("0.1"), 2},
                {tr("0.01"), 3},
                {tr("0.001"), 4},
                {tr("0.0001"), 5},
                {tr("0.00001"), 6},
                {tr("0.000001"), 7},
                {tr("0.0000001"), 8},
                {tr("0.00000001"), 9}
            });
        }

        entity->addBooleanDS("dimTolSuppressLeading", [](const LC_DimStyle* ds) -> bool {
                                 return ds->zerosSuppression()->isToleranceSuppress(
                                     LC_DimStyle::ZerosSuppression::ToleranceSuppressionPolicy::SUPPRESS_LEADING_ZEROS);
                             }, tr("Tolerance suppress leading zeros"),
                             tr("Sets suppression of leading zeros for tolerance values in dimension (DIMTZIN value)"));

        entity->addBooleanDS("dimTolSuppressTrailing", [](const LC_DimStyle* ds) -> bool {
                                 return ds->zerosSuppression()->isToleranceSuppress(
                                     LC_DimStyle::ZerosSuppression::ToleranceSuppressionPolicy::SUPPRESS_TRAILING_ZEROS);
                             }, tr("Tolerance suppress trailing zeros"),
                             tr("Sets suppression of trailing zeros for tolerance values in dimension (DIMTZIN value)"));

        if (notAngularDimension) {
            entity->addBooleanDS("dimTolSuppressZeroFeet", [](const LC_DimStyle* ds) -> bool {
                                     const auto zerosSuppression = ds->zerosSuppression();
                                     bool feetSuppress = false;
                                     if (zerosSuppression->isToleranceSuppress(
                                         LC_DimStyle::ZerosSuppression::ToleranceSuppressionPolicy::TOL_INCLUDE_ZERO_FEET_AND_ZERO_INCHES)) {
                                         if (zerosSuppression->isToleranceSuppress(
                                             LC_DimStyle::ZerosSuppression::ToleranceSuppressionPolicy::TOL_INCLUDE_ZERO_FEET_AND_SUPPRESS_ZERO_INCHES)) {
                                             feetSuppress = true;
                                         }
                                     }
                                     else {
                                         if (!zerosSuppression->isLinearSuppress(
                                             LC_DimStyle::ZerosSuppression::ToleranceSuppressionPolicy::TOL_INCLUDE_ZERO_FEET_AND_SUPPRESS_ZERO_INCHES)) {
                                             feetSuppress = true;
                                         }
                                     }
                                     return feetSuppress;
                                 }, tr("Tolerance suppress zero feet"),
                                 tr("Sets suppression of zero feet for tolerance values in dimension (DIMTZIN variable)"));

            entity->addBooleanDS("dimTolSuppressZeroInch", [](const LC_DimStyle* ds) -> bool {
                                     const auto zerosSuppression = ds->zerosSuppression();
                                     const bool inchesSuppress = zerosSuppression->
                                         isToleranceSuppress(LC_DimStyle::ZerosSuppression::TOL_INCLUDE_ZERO_FEET_AND_ZERO_INCHES) && !
                                         zerosSuppression->
                                         isToleranceSuppress(LC_DimStyle::ZerosSuppression::TOL_INCLUDE_ZERO_FEET_AND_ZERO_INCHES);
                                     return inchesSuppress;
                                 }, tr("Tolerance suppress zero inches"),
                                 tr("Sets suppression of zero inches for tolerance values in dimension (DIMTZIN variable)"));
        }

        entity->addDoubleDS("dimTolTextHeight", [](const LC_DimStyle* ds) -> double {
                                return ds->latteralTolerance()->heightScaleFactorToDimText();
                            }, tr("Tolerance text height"),
                            tr("Specifies scale factor for text height of tolerance values relative to dimension text "
                                "height as set by DIMTXT (DIMTFAC variable)"));

        if (notAngularDimension) {
            entity->addIntEnumDS("dimTolAltPrecision", [](const LC_DimStyle* ds) -> int {
                                     const auto tol = ds->latteralTolerance();
                                     const auto places = tol->decimalPlacesAltDim();
                                     return places;
                                 }, tr("Alt tolerance precision"),
                                 tr("Specifies number of decimal places for tolerance values of an "
                                     "alternate units dimension (DIMALTTD variable)"), {
                                     {tr("1.00"), 1},
                                     {tr("0.1"), 2},
                                     {tr("0.01"), 3},
                                     {tr("0.001"), 4},
                                     {tr("0.0001"), 5},
                                     {tr("0.00001"), 6},
                                     {tr("0.000001"), 7},
                                     {tr("0.0000001"), 8},
                                     {tr("0.00000001"), 9}
                                 });

            entity->addBooleanDS("dimTolAltSuppressLeading", [](const LC_DimStyle* ds) -> bool {
                                     return ds->zerosSuppression()->isAltToleranceSuppress(
                                         LC_DimStyle::ZerosSuppression::ToleranceSuppressionPolicy::SUPPRESS_LEADING_ZEROS);
                                 }, tr("Alt tolerance suppress leading zeros"),
                                 tr(
                                     "Sets suppression of leading zeros for alternate units tolerance values in dimension (DIMALTTZ value)"));

            entity->addBooleanDS("dimTolAltSuppressTrailing", [](const LC_DimStyle* ds) -> bool {
                                     return ds->zerosSuppression()->isAltToleranceSuppress(
                                         LC_DimStyle::ZerosSuppression::ToleranceSuppressionPolicy::SUPPRESS_TRAILING_ZEROS);
                                 }, tr("Alt tolerance suppress trailing zeros"),
                                 tr(
                                     "Sets suppression of trailing zeros for alternate units tolerance values in dimension (DIMALTTZ value)"));

            entity->addBooleanDS("dimTolAltSuppressZeroFeet", [](const LC_DimStyle* ds) -> bool {
                                     const auto zerosSuppression = ds->zerosSuppression();
                                     bool feetSuppress = false;
                                     if (zerosSuppression->isAltToleranceSuppress(
                                         LC_DimStyle::ZerosSuppression::ToleranceSuppressionPolicy::TOL_INCLUDE_ZERO_FEET_AND_ZERO_INCHES)) {
                                         if (zerosSuppression->isAltToleranceSuppress(
                                             LC_DimStyle::ZerosSuppression::ToleranceSuppressionPolicy::TOL_INCLUDE_ZERO_FEET_AND_SUPPRESS_ZERO_INCHES)) {
                                             feetSuppress = true;
                                         }
                                     }
                                     else {
                                         if (!zerosSuppression->isAltToleranceSuppress(
                                             LC_DimStyle::ZerosSuppression::ToleranceSuppressionPolicy::TOL_INCLUDE_ZERO_FEET_AND_SUPPRESS_ZERO_INCHES)) {
                                             feetSuppress = true;
                                         }
                                     }
                                     return feetSuppress;
                                 }, tr("Alt tolerance suppress zero feet"),
                                 tr("Sets suppression of zero feet for alternate units tolerance values in dimension (DIMALTTZ variable)"));

            entity->addBooleanDS("dimTolAltSuppressZeroInch", [](const LC_DimStyle* ds) -> bool {
                                     const auto zerosSuppression = ds->zerosSuppression();
                                     const bool inchesSuppress = zerosSuppression->isAltToleranceSuppress(
                                         LC_DimStyle::ZerosSuppression::TOL_INCLUDE_ZERO_FEET_AND_ZERO_INCHES) && !zerosSuppression->
                                     isAltToleranceSuppress(
                                         LC_DimStyle::ZerosSuppression::TOL_INCLUDE_ZERO_FEET_AND_ZERO_INCHES);
                                     return inchesSuppress;
                                 }, tr("Alt tolerance suppress zero inches"),
                                 tr(
                                     "Sets suppression of zero inches for alterate units tolerance values in dimension (DIMALTTZ variable)"));
        }
    }

    /*
        * todo: probably later on it will be possible to show the combobox based on the current format of the document,
        * so let this code be there for a while
        *

    template <typename EntityType>
    static void addDecimalPlacesFieldDS(const QString& name, const std::function<int(LC_DimStyle* ds)>& funGetValue,
                                        const QString& displayName, const QString& description,
                                        LC_DimensionEntityMatchDescriptor<EntityType>* entity
    //                                   , RS2::LinearFormat linearFormat
    ) {

        entity->addIntEnumDS(name, funGetValue, displayName, description, {
                                        {tr("1.00"), 1},
                                        {tr("0.1"), 2},
                                        {tr("0.01"), 3},
                                        {tr("0.001"), 4},
                                        {tr("0.0001"), 5},
                                        {tr("0.00001"), 6},
                                        {tr("0.000001"), 7},
                                        {tr("0.0000001"), 8},
                                        {tr("0.00000001"), 9}
                                    });

         *switch (linearFormat) {
            case (RS2::Scientific):
                entity->addIntEnumDS(name, funGetValue, displayName, description, {
                                         {tr("1E+01"), 1},
                                         {tr("1E-1"), 2},
                                         {tr("1E-2"), 3},
                                         {tr("1E-3"), 4},
                                         {tr("1E-4"), 5},
                                         {tr("1E-5"), 6},
                                         {tr("1E-6"), 7},
                                         {tr("1E-7"), 8},
                                         {tr("1E-8"), 9}
                                     });
                break;
            case RS2::ArchitecturalMetric:
            case RS2::Decimal:
                entity->addIntEnumDS(name, funGetValue, displayName, description, {
                                         {tr("1.00"), 1},
                                         {tr("0.1"), 2},
                                         {tr("0.01"), 3},
                                         {tr("0.001"), 4},
                                         {tr("0.0001"), 5},
                                         {tr("0.00001"), 6},
                                         {tr("0.000001"), 7},
                                         {tr("0.0000001"), 8},
                                         {tr("0.00000001"), 9}
                                     });
                break;
            case RS2::Engineering:
                entity->addIntEnumDS(name, funGetValue, displayName, description, {
                                         {tr("0'-0\""), 1},
                                         {tr("0'-0.0\""), 2},
                                         {tr("0'-0.00\""), 3},
                                         {tr("0'-0.000\""), 4},
                                         {tr("0'-0.0000\""), 5},
                                         {tr("0'-0.00000\""), 6},
                                         {tr("0'-0.000000\""), 7},
                                         {tr("0'-0.0000000\""), 8},
                                         {tr("0'-0.00000000\""), 9}
                                     });
            case RS2::Fractional:
                entity->addIntEnumDS(name, funGetValue, displayName, description, {
                                         {tr("1"), 1},
                                         {tr("0 1/2"), 2},
                                         {tr("0 1/4"), 3},
                                         {tr("0 1/8"), 4},
                                         {tr("0 1/16"), 5},
                                         {tr("0 1/32"), 6},
                                         {tr("0 1/64"), 7},
                                         {tr("0 1/128"), 8},
                                         {tr("0 1/256"), 9}
                                     });
            case RS2::Architectural:
                entity->addIntEnumDS(name, funGetValue, displayName, description, {
                                         {tr("0'-0\""), 1},
                                         {tr("0'-0 1/2\""), 2},
                                         {tr("0'-0 1/4\""), 3},
                                         {tr("0'-0 1/8\""), 4},
                                         {tr("0'-0 1/16\""), 5},
                                         {tr("0'-0 1/32\""), 6},
                                         {tr("0'-0 1/64\""), 7},
                                         {tr("0'-0 1/128\""), 8},
                                         {tr("0'-0 1/256\""), 9}
                                     });
            default:
                entity->addIntEnumDS(name, funGetValue, displayName, description, {
                                         {tr("1.00"), 1},
                                         {tr("0.1"), 2},
                                         {tr("0.01"), 3},
                                         {tr("0.001"), 4},
                                         {tr("0.0001"), 5},
                                         {tr("0.00001"), 6},
                                         {tr("0.000001"), 7},
                                         {tr("0.0000001"), 8},
                                         {tr("0.00000001"), 9}
                                     });
        }
}*/

/*
    * todo: probably later on it will be possible to show the combobox based on the current format of the document,
    * so let this code be there for a while
    *
template <typename EntityType>
static void addAngleDecimalPlacesField_DS(const QString& name,const std::function<int(LC_DimStyle* ds)>& funGetValue,
                                  const QString& displayName, const QString& description,
                                  LC_DimensionEntityMatchDescriptor<EntityType>* entity,
                                  //RS2::AngleFormat angleFormat*
                                  ) {

    entity->addIntEnumDS(name, funGetValue, displayName, description, {
                                    {tr("1.00"), 1},
                                    {tr("0.10"), 2},
                                    {tr("0.01"), 3},
                                    {tr("0.001"), 4},
                                    {tr("0.0001"), 5},
                                    {tr("0.00001"), 6},
                                    {tr("0.000001"), 7},
                                    {tr("0.0000001"), 8},
                                    {tr("0.00000001"), 9}
                                });

    switch (angleFormat) {
        case (RS2::DegreesDecimal):
            entity->addIntEnumDS(name, funGetValue, displayName, description, {
                                     {tr("1.00"), 1},
                                     {tr("0.10"), 2},
                                     {tr("0.01"), 3},
                                     {tr("0.001"), 4},
                                     {tr("0.0001"), 5},
                                     {tr("0.00001"), 6},
                                     {tr("0.000001"), 7},
                                     {tr("0.0000001"), 8},
                                     {tr("0.00000001"), 9}
                                 });
            break;
        case RS2::DegreesMinutesSeconds:
            entity->addIntEnumDS(name, funGetValue, displayName, description, {
                                     {QString("0%1").arg(QChar(0xB0)), 1},
                                     {QString("0%100'").arg(QChar(0xB0)), 2},
                                     {QString("0%100'00\"").arg(QChar(0xB0)), 3},
                                     {QString("0%100'00.0\"").arg(QChar(0xB0)), 4},
                                     {QString("0%100'00.00\"").arg(QChar(0xB0)), 5},
                                     {QString("0%100'00.000\"").arg(QChar(0xB0)), 6},
                                     {QString("0%100'00.0000\"").arg(QChar(0xB0)), 7},
                                     {QString("0%100'00.00000\"").arg(QChar(0xB0)), 8},
                                     {QString("0%100'00.000000\"").arg(QChar(0xB0)), 9}
                                 });
        case RS2::Gradians:
            entity->addIntEnumDS(name, funGetValue, displayName, description, {
                                     {tr("1g"), 1},
                                     {tr("0.1g"), 2},
                                     {tr("0.01g"), 3},
                                     {tr("0.001g"), 4},
                                     {tr("0.0001g"), 5},
                                     {tr("0.00001g"), 6},
                                     {tr("0.000001g"), 7},
                                     {tr("0.0000001g"), 8},
                                     {tr("0.00000001g"), 9}
                                 });
            break;
        case RS2::Radians:
            entity->addIntEnumDS(name, funGetValue, displayName, description, {
                                     {tr("1r"), 1},
                                     {tr("0.1r"), 2},
                                     {tr("0.01r"), 3},
                                     {tr("0.001r"), 4},
                                     {tr("0.0001r"), 5},
                                     {tr("0.00001r"), 6},
                                     {tr("0.000001r"), 7},
                                     {tr("0.0000001r"), 8},
                                     {tr("0.00000001r"), 9}
                                 });
            break;
        case RS2::Surveyors:
            entity->addIntEnumDS(name, funGetValue, displayName, description, {
                                     {tr("N 1d E"), 1},
                                     {tr("N 0d01' E"), 2},
                                     {tr("N 0d00'01\" E"), 3},
                                     {tr("N 0d00'00.1\" E"), 4},
                                     {tr("N 0d00'00.01\" E"), 5},
                                     {tr("N 0d00'00.001\" E"), 6},
                                     {tr("N 0d00'00.0001\" E"), 7},
                                     {tr("N 0d00'00.00001\" E"), 8},
                                     {tr("N 0d00'00.000001\" E"), 9}
                                 });
            break;
        default:
            entity->addIntEnumDS(name, funGetValue, displayName, description, {
                                     {tr("1.00"), 1},
                                     {tr("0.10"), 2},
                                     {tr("0.01"), 3},
                                     {tr("0.001"), 4},
                                     {tr("0.0001"), 5},
                                     {tr("0.00001"), 6},
                                     {tr("0.000001"), 7},
                                     {tr("0.0000001"), 8},
                                     {tr("0.00000001"), 9}
                                 });
    }
}*/

};
#endif
