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

#include "lc_propertiesprovider_dim_base.h"

#include "lc_dlgdimstylemanager.h"
#include "lc_property_action.h"
#include "lc_property_action_link_view.h"
#include "lc_property_double_spinbox_view.h"
#include "lc_property_linetype.h"
#include "lc_property_linewidth.h"
#include "lc_property_qstring_list_arrows_combobox_view.h"
#include "lc_property_rscolor.h"
#include "lc_propertyprovider_utils.h"
#include "qc_applicationwindow.h"
#include "rs_block.h"
#include "rs_dimension.h"

const QString LC_PropertiesProviderDimBase::SECTION_DIM_GENERAL = "_secDimGeneral";
const QString LC_PropertiesProviderDimBase::SECTION_DIM_LINES_AND_ARROWS = "_secDimLinesArrows";
const QString LC_PropertiesProviderDimBase::SECTION_DIM_TEXT = "_secDimText";
const QString LC_PropertiesProviderDimBase::SECTION_DIM_FIT = "_secDimFit";
const QString LC_PropertiesProviderDimBase::SECTION_DIM_PRIMARY_UNITS = "_secDimPrimaryUnits";
const QString LC_PropertiesProviderDimBase::SECTION_DIM_ALTERNATE_UNITS = "_secDimAlternateUnits";
const QString LC_PropertiesProviderDimBase::SECTION_DIM_TOLERANCES = "_secDimTolerances";
const QString LC_PropertiesProviderDimBase::SECTION_DIM_GEOMETRY = "_secDimTolerances";

void LC_PropertiesProviderDimBase::doCreateEntitySpecificProperties(LC_PropertyContainer* container, const QList<RS_Entity*>& list) {
    createMiscSection(container, list);
    createDimGeometrySection(container, list);
    createLinesAndArrowsSection(container, list);
    createTextSection(container, list);
    createFitSection(container, list);
    createPrimaryUnitsSection(container, list);
    if (m_entityType != RS2::EntityDimAngular) {
        createAlternateUnitsSection(container, list);
    }
    createTolerancesSection(container, list);
}

void LC_PropertiesProviderDimBase::createMiscSection(LC_PropertyContainer* container, const QList<RS_Entity*>& list) {
    const auto cont = createSection(container, {SECTION_DIM_GENERAL, tr("Misc"), tr("Misc dimension data")});
    addStringList<RS_Dimension>({"dimStyle", tr("Style"), tr("Specifies current dimension style by name")},
                                [](const RS_Dimension* e) -> QString {
                                    return e->getStyle();
                                }, [](const QString& v, RS_Dimension* l) -> void {
                                    l->setStyle(v);
                                }, [this]([[maybe_unused]] RS_Dimension* h, LC_PropertyViewDescriptor& descriptor)-> bool {
                                    const auto graphic = this->m_actionContext->getDocument()->getGraphic();
                                    if (graphic != nullptr) {
                                        QStringList values;
                                        const auto dimStylesList = graphic->getDimStyleList();
                                        const auto stylesList = dimStylesList->getStylesList();
                                        for (const auto b : *stylesList) {
                                            if (b->isBaseStyle()) {
                                                values.append(b->getName());
                                            }
                                        }
                                        values.sort();
                                        descriptor[LC_PropertyQStringListComboBoxView::ATTR_ITEMS] = values;
                                        return values.isEmpty();
                                    }
                                    return true;
                                }, list, cont);

    bool hasOverride = false;
    for (const auto e : list) {
        const auto* d = static_cast<RS_Dimension*>(e);
        if (d->getDimStyleOverride() != nullptr) {
            hasOverride = true;
            break;
        }
    }

    if (hasOverride) {
        auto* property = new LC_PropertyAction(cont, true);
        const LC_Property::Names names = {
            "dimClearOverride",
            tr("Clear dim style override"),
            tr("Clears dimension style override, if one is present for dimension")
        };
        property->setNames(names);
        LC_PropertyViewDescriptor viewDescriptor("Link");
        viewDescriptor[LC_PropertyActionLinkView::ATTR_TITLE] = names.displayName;
        property->setClickHandler([this, property, list]([[maybe_unused]] const LC_PropertyAction* action, [[maybe_unused]] int linkIndex) {
            for (const auto e : list) {
                const auto d = dynamic_cast<RS_Dimension*>(e);
                if (d != nullptr) {
                    if (d->getDimStyleOverride() != nullptr) {
                        const auto clone = static_cast<RS_Dimension*>(d->clone());
                        clone->setDimStyleOverride(nullptr);
                        m_widget->entityModified(d, clone);
                    }
                }
            }
            m_widget->onPropertyEdited(property);
        });

        property->setViewDescriptor(viewDescriptor);
        cont->addChildProperty(property);
    }

    if (isShowLinks()) {
        auto clickHandler = []([[maybe_unused]] RS_Dimension* entity, [[maybe_unused]]const int linkIndex) {
            QC_ApplicationWindow::getAppWindow()->changeDrawingOptions(3);
        };
        LC_PropertyProviderUtils::createSingleEntityCommand<RS_Dimension>(cont, "propertyName", tr("Manage styles.."),
                                                                       tr("Invokes dimension styles management UI"), "",
                                                                       "", nullptr,
                                                                       clickHandler, tr("Dimension styles management"));
    }
}

void LC_PropertiesProviderDimBase::addArrowsFlipLinks(const QList<RS_Entity*>& list, LC_PropertyContainer* cont) const {
    const bool flipArrowEnabled = m_entityType != RS2::EntityDimOrdinate;
    // todo - probably it's better to use two columns for flipArrow commands?
    if (flipArrowEnabled) {
        auto* flipArrow1 = new LC_PropertyAction(cont, true);
        const LC_Property::Names names1 = {"dimFlipArrow1", tr("Flip Arrow 1"), tr("Flips direction of arrows 1, if any")};
        flipArrow1->setNames(names1);
        LC_PropertyViewDescriptor viewDescriptor1("Link");
        viewDescriptor1["title"] = names1.displayName;
        flipArrow1->setClickHandler(
            [this, flipArrow1, list]([[maybe_unused]] const LC_PropertyAction* action, [[maybe_unused]] int linkIndex) {
                for (const auto e : list) {
                    const auto d = dynamic_cast<RS_Dimension*>(e);
                    if (d != nullptr) {
                        const auto clone = static_cast<RS_Dimension*>(d->clone());
                        const bool flip = clone->isFlipArrow1();
                        clone->setFlipArrow1(!flip);
                        m_widget->entityModified(d, clone);
                    }
                }
                m_widget->onPropertyEdited(flipArrow1);
            });

        flipArrow1->setViewDescriptor(viewDescriptor1);
        cont->addChildProperty(flipArrow1);

        if (m_entityType != RS2::EntityDimRadial) {
            auto* flipArrow2 = new LC_PropertyAction(cont, true);
            const LC_Property::Names names2 = {"dimFlipArrow2", tr("Flip Arrow 2"), tr("Flips direction of arrows 2, if any")};
            flipArrow2->setNames(names2);
            LC_PropertyViewDescriptor viewDescriptor2("Link");
            viewDescriptor2["title"] = names2.displayName;
            flipArrow2->setClickHandler(
                [this, flipArrow2, list]([[maybe_unused]] const LC_PropertyAction* action, [[maybe_unused]] int linkIndex) {
                    for (const auto e : list) {
                        const auto d = dynamic_cast<RS_Dimension*>(e);
                        if (d != nullptr) {
                            const auto clone = static_cast<RS_Dimension*>(d->clone());
                            const bool flip = clone->isFlipArrow2();
                            clone->setFlipArrow2(!flip);
                            m_widget->entityModified(d, clone);
                        }
                    }
                    m_widget->onPropertyEdited(flipArrow2);
                });

            flipArrow2->setViewDescriptor(viewDescriptor2);
            cont->addChildProperty(flipArrow2);
        }
    }
}

void LC_PropertiesProviderDimBase::prepareArrowheadItemsDescriptor(QString arrowName,
                                                                   const std::vector<LC_DimArrowRegistry::ArrowInfo>& arrowTypes,
                                                                   const bool hasCustomBlocks, const QStringList& blocksNames,
                                                                   LC_PropertyViewDescriptor& descriptor) {
    if (arrowName.isEmpty()) {
        arrowName = "_CLOSEDFILLED";
    }

    int currentIndex = -1;

    QStringList icons;
    QStringList items;
    QStringList data;
    int idx = 0;
    for (const LC_DimArrowRegistry::ArrowInfo& arrowInfo : arrowTypes) {
        auto arrowBlockName = arrowInfo.blockName;
        QString blockName = arrowBlockName.toLower();
        QString iconName = ":/arrows/arrow" + blockName + ".lci";
        icons.push_back(iconName);
        items.push_back(arrowInfo.name);
        if (arrowName == arrowBlockName) {
            currentIndex = idx;
        }
        data.push_back(arrowBlockName);
        idx++;
    }
    if (hasCustomBlocks) {
        if (currentIndex == -1) {
            // it seems it's custom block
            icons.push_back("");
            items.push_back(arrowName);
            data.push_back(arrowName);
            currentIndex = idx;
        }
        icons.push_back("");
        items.push_back(tr("User Block..."));
        data.push_back("_CUSTOM_SELECT");
    }
    descriptor.viewName = LC_PropertyQStringListArrowsComboboxView::VIEW_NAME;
    descriptor.attributes[LC_PropertyQStringListComboBoxView::ATTR_ITEMS_ICON_NAMES] = icons;
    descriptor.attributes[LC_PropertyQStringListComboBoxView::ATTR_ITEMS] = items;
    descriptor.attributes[LC_PropertyQStringListComboBoxView::ATTR_ITEMS_DATA] = data;
    descriptor.attributes[LC_PropertyQStringListArrowsComboboxView::ATTR_BLOCK_NAMES] = blocksNames;
    descriptor.attributes[LC_PropertyQStringListArrowsComboboxView::ATTR_CURRENT_INDEX] = currentIndex;
}

void LC_PropertiesProviderDimBase::createLinesAndArrowsSection(LC_PropertyContainer* container, const QList<RS_Entity*>& list) {
    const auto cont = createSection(container, {
                                        SECTION_DIM_LINES_AND_ARROWS,
                                        tr("Lines and Arrows"),
                                        tr("Dimensions lines and arrows setup")
                                    });

    auto funPositiveDoubleSpin = []([[maybe_unused]] RS_Dimension* dimension, LC_PropertyViewDescriptor& descriptor)-> bool {
        descriptor.attributes[LC_PropertyDoubleSpinBoxView::ATTR_MIN] = 0.0;
        descriptor.attributes[LC_PropertyDoubleSpinBoxView::ATTR_STEP] = 0.01;
        return false;
    };

    std::vector<LC_DimArrowRegistry::ArrowInfo> arrowTypes;
    LC_DimArrowRegistry::fillDefaultArrowTypes(arrowTypes);
    const auto graphic = m_actionContext->getDocument()->getGraphic();
    bool hasCustomBlocks = false;

    QStringList blocksNames;

    const auto blocksList = graphic->getBlockList();
    const int blocksCount = blocksList->count();
    if (blocksCount > 0) {
        for (const RS_Block* block : *blocksList) {
            QString blockName = block->getName();
            blocksNames << blockName;
        }
        blocksNames.sort();
        hasCustomBlocks = true;
    }

    const bool notDimOrdinate = m_entityType != RS2::EntityDimOrdinate;
    if (notDimOrdinate) {
        addStringList<RS_Dimension>({"dimArrow1", tr("Arrow 1"), tr("Specifies type of the first dimension arrowhead (DIMBLK1 variable)")},
                                    [](RS_Dimension* e) -> QString {
                                        const LC_DimStyle* ds = e->getEffectiveCachedDimStyle();
                                        const auto arrowhead = ds->arrowhead();
                                        auto chars = arrowhead->obtainFirstArrowName();
                                        return chars;
                                    }, [](const QString& v, RS_Dimension* e) -> void {
                                        const LC_DimStyle* ds = e->getEffectiveDimStyleOverride();
                                        const auto arrowhead = ds->arrowhead();
                                        arrowhead->setArrowHeadBlockNameFirst(v);
                                        if (!arrowhead->isUseSeparateArrowHeads()) {
                                            arrowhead->setArrowHeadBlockNameSecond(v);
                                        }
                                    }, [this, arrowTypes, hasCustomBlocks, blocksNames]([[maybe_unused]] RS_Dimension* e,
                                                                                        LC_PropertyViewDescriptor& descriptor)-> bool {
                                        const LC_DimStyle* ds = e->getEffectiveCachedDimStyle();
                                        const auto arrowhead = ds->arrowhead();
                                        const auto arrowName = arrowhead->obtainFirstArrowName();
                                        prepareArrowheadItemsDescriptor(arrowName, arrowTypes, hasCustomBlocks, blocksNames, descriptor);
                                        return false;
                                    }, list, cont);

        addStringList<RS_Dimension>({"dimArrow2", tr("Arrow 2"), tr("Specifies type of the second dimension arrowhead (DIMBLK2 variable)")},
                                    [](RS_Dimension* e) -> QString {
                                        const LC_DimStyle* ds = e->getEffectiveCachedDimStyle();
                                        const auto arrowhead = ds->arrowhead();
                                        auto chars = arrowhead->obtainSecondArrowName();
                                        return chars;
                                    }, [](const QString& v, RS_Dimension* e) -> void {
                                        const LC_DimStyle* ds = e->getEffectiveDimStyleOverride();
                                        const auto arrowhead = ds->arrowhead();
                                        if (arrowhead->isUseSeparateArrowHeads()) {
                                            arrowhead->setArrowHeadBlockNameSecond(v);
                                        }
                                        else {
                                            if (v != arrowhead->arrowHeadBlockNameFirst()) {
                                                arrowhead->setUseSeparateArrowHeads(true);
                                                arrowhead->setArrowHeadBlockNameSecond(v);
                                            }
                                        }
                                    }, [this, arrowTypes, hasCustomBlocks, blocksNames]([[maybe_unused]] RS_Dimension* e,
                                                                                        LC_PropertyViewDescriptor& descriptor)-> bool {
                                        const LC_DimStyle* ds = e->getEffectiveCachedDimStyle();
                                        const auto arrowhead = ds->arrowhead();
                                        const auto arrowName = arrowhead->obtainSecondArrowName();
                                        prepareArrowheadItemsDescriptor(arrowName, arrowTypes, hasCustomBlocks, blocksNames, descriptor);
                                        return false;
                                    }, list, cont);

        addArrowsFlipLinks(list, cont);
    }

    addDouble_DS({"dimArrowSize", tr("Arrow size"), tr("Specifies size of the dimension arrowhead (DIMASZ variable)")},
                 [](const LC_DimStyle* ds) -> double {
                     return ds->arrowhead()->size();
                 }, [](const double& v, const LC_DimStyle* ds) -> void {
                     ds->arrowhead()->setSize(v);
                 }, list, cont, funPositiveDoubleSpin);

    const bool radialOrDiametric = m_entityType == RS2::EntityDimRadial || m_entityType == RS2::EntityDimDiametric;
    if (radialOrDiametric) {
        static LC_EnumDescriptor dimjustPolicyDescriptor = {
            "DIMCENPolicy",
            {
                {LC_DimStyle::Radial::DRAW_CENTERMARKS, tr("Mark")},
                {LC_DimStyle::Radial::DRAW_CENTERLINES, tr("Line")},
                {LC_DimStyle::Radial::DRAW_NOTHING, tr("None")}
            }
        };
        addEnum_DS({"dimCenterMark", tr("Center mark"), tr("Specified type of center mark on dimension (DIMCEN variable)")},
                   &dimjustPolicyDescriptor, [](const LC_DimStyle* ds) -> LC_PropertyEnumValueType {
                       return ds->radial()->drawingMode();
                   }, [](const LC_PropertyEnumValueType& v, const LC_DimStyle* ds) -> void {
                       double size = ds->radial()->size();

                       switch (v) {
                           case LC_DimStyle::Radial::DRAW_CENTERMARKS: {
                               break;
                           }
                           case LC_DimStyle::Radial::DRAW_CENTERLINES: {
                               size = -size;
                               break;
                           }
                           case LC_DimStyle::Radial::DRAW_NOTHING: {
                               size = 0.0;
                               break;
                           }
                           default:
                               size = 0.0;
                       }
                       ds->radial()->setCenterMarkOrLineSize(size);
                   }, list, cont);

        addDouble_DS({
                         "dimCentermarkSize",
                         tr("Center mark size"),
                         tr("Specifies size of the center mark on the dimension (DIMCEN variable)")
                     }, [](const LC_DimStyle* ds) -> double {
                         return ds->radial()->size();
                     }, [](const double& v, const LC_DimStyle* ds) -> void {
                         const auto centerMarkDrawingMode = ds->radial()->drawingMode();
                         double size = v;
                         switch (centerMarkDrawingMode) {
                             case LC_DimStyle::Radial::DRAW_CENTERMARKS: {
                                 break;
                             }
                             case LC_DimStyle::Radial::DRAW_CENTERLINES: {
                                 size = -size;
                                 break;
                             }
                             case LC_DimStyle::Radial::DRAW_NOTHING: {
                                 size = 0.0;
                                 break;
                             }
                             default:
                                 size = 0.0;
                         }
                         ds->radial()->setCenterMarkOrLineSize(size);
                     }, list, cont, funPositiveDoubleSpin);
    }

    if (notDimOrdinate) {
        addLineWidth_DS({"dimLineWeight", tr("Dim line lineweight"), tr("Specifies lineweight for dimension lines (DIMLWD variable)")},
                        [](const LC_DimStyle* ds) -> RS2::LineWidth {
                            return ds->dimensionLine()->lineWidth();
                        }, [](const RS2::LineWidth& l, const LC_DimStyle* ds) -> void {
                            ds->dimensionLine()->setLineWidth(l);
                        }, list, cont);

        if (!radialOrDiametric) {
            addLineWidth_DS(
                {"dimExtLineWeight", tr("Ext line lineweight"), tr("Specifies lineweight for extension line (DIMLWE variable)")},
                [](const LC_DimStyle* ds) -> RS2::LineWidth {
                    return ds->extensionLine()->lineWidth();
                }, [](const RS2::LineWidth& l, const LC_DimStyle* ds) -> void {
                    ds->extensionLine()->setLineWidth(l);
                }, list, cont);
        }

        addBoolean_DS({"dimShowDim1", tr("Dim line 1"), tr("Sets suppression of first dimension line (DIMSD1 variable)")},
                      [](const LC_DimStyle* ds) -> bool {
                          return !ds->dimensionLine()->isSuppressFirst();
                      }, [](const bool& v, const LC_DimStyle* ds) -> void {
                          ds->dimensionLine()->setSuppressFirst(!v);
                      }, list, cont);

        addBoolean_DS({"dimShowDim2", tr("Dim line 2"), tr("Sets suppression of second dimension line (DIMSD2 variable)")},
                      [](const LC_DimStyle* ds) -> bool {
                          return !ds->dimensionLine()->isSuppressSecond();
                      }, [](const bool& v, const LC_DimStyle* ds) -> void {
                          ds->dimensionLine()->setSuppressSecond(!v);
                      }, list, cont);

        addColor_DS({"dimLineColor", tr("Dim line color"), tr("Specifies color of the dimension line (DIMCLRD variable)")},
                    [](const LC_DimStyle* ds) -> RS_Color {
                        const auto text = ds->dimensionLine();
                        return text->color();
                    }, [](const RS_Color& color, const LC_DimStyle* ds) -> void {
                        const auto text = ds->dimensionLine();
                        text->setColor(color);
                    }, list, cont);

        addLineType_DS({"dimLineLineType", tr("Dim line linetype"), tr("Specifies the linetype of the dimension line (DIMLTYPE variable)")},
                       [](const LC_DimStyle* ds) -> RS2::LineType {
                           return ds->dimensionLine()->lineType();
                       }, [](const RS2::LineType& linetype, const LC_DimStyle* ds) -> void {
                           const auto line = ds->dimensionLine();
                           line->setLineType(linetype);
                       }, list, cont);

        if (m_entityType != RS2::EntityDimAngular) {
            if (!radialOrDiametric/* && m_entityType != RS2::EntityDimArc*/) {
                addDouble_DS({
                                 "dimLineExt",
                                 tr("Dim line ext"),
                                 tr("Specifies amount to extend dimension lines beyond the extension lines (DIMDLE variable)")
                             }, [](const LC_DimStyle* ds) -> double {
                                 return ds->text()->height();
                             }, [](const double& v, const LC_DimStyle* ds) -> void {
                                 ds->text()->setHeight(v);
                             }, list, cont, funPositiveDoubleSpin);
            }
        }
        if (radialOrDiametric) {
            addBoolean_DS({"dimExt1Show", tr("Ext line"), tr("Sets suppression of extension line (DIMSE1 variable)")},
                          [](const LC_DimStyle* ds) -> bool {
                              return !ds->extensionLine()->isSuppressFirst();
                          }, [](const bool& v, const LC_DimStyle* ds) -> void {
                              ds->extensionLine()->setSuppressFirst(!v);
                          }, list, cont);

            addLineWidth_DS(
                {"dimExtLineWeight", tr("Ext line lineweight"), tr("Specifies lineweight for extension line (DIMLWE variable)")},
                [](const LC_DimStyle* ds) -> RS2::LineWidth {
                    return ds->extensionLine()->lineWidth();
                }, [](const RS2::LineWidth& l, const LC_DimStyle* ds) -> void {
                    ds->extensionLine()->setLineWidth(l);
                }, list, cont);

            addLineType_DS({
                               "dimExtLineType",
                               tr("Ext line linetype"),
                               tr("Specifies the linetype of the extension line (DIMLTEX1 variable)")
                           }, [](const LC_DimStyle* ds) -> RS2::LineType {
                               return ds->extensionLine()->lineTypeFirst();
                           }, [](const RS2::LineType& linetype, const LC_DimStyle* ds) -> void {
                               const auto line = ds->extensionLine();
                               line->setLineTypeFirst(linetype);
                           }, list, cont);
        }
        else {
            addLineType_DS({
                               "dimExt1LineType",
                               tr("Ext line 1 linetype"),
                               tr("Specifies the linetype of the first extension line (DIMLTEX1 variable)")
                           }, [](const LC_DimStyle* ds) -> RS2::LineType {
                               return ds->extensionLine()->lineTypeFirst();
                           }, [](const RS2::LineType& linetype, const LC_DimStyle* ds) -> void {
                               const auto line = ds->extensionLine();
                               line->setLineTypeFirst(linetype);
                           }, list, cont);

            addLineType_DS({
                               "dimExt2LineType",
                               tr("Ext line 2 linetype"),
                               tr("Specifies the linetype of the second extension line (DIMLTEX2 variable)")
                           }, [](const LC_DimStyle* ds) -> RS2::LineType {
                               return ds->extensionLine()->lineTypeSecond();
                           }, [](const RS2::LineType& linetype, const LC_DimStyle* ds) -> void {
                               const auto line = ds->extensionLine();
                               line->setLineTypeSecond(linetype);
                           }, list, cont);

            addBoolean_DS({"dimExt1Show", tr("Ext line 1"), tr("Sets suppression of first extension line (DIMSE1 variable)")},
                          [](const LC_DimStyle* ds) -> bool {
                              return !ds->extensionLine()->isSuppressFirst();
                          }, [](const bool& v, const LC_DimStyle* ds) -> void {
                              ds->extensionLine()->setSuppressFirst(!v);
                          }, list, cont);

            addBoolean_DS({"dimExt2Show", tr("Ext line 2"), tr("Sets suppression of second extension line (DIMSE2 variable)")},
                          [](const LC_DimStyle* ds) -> bool {
                              return !ds->extensionLine()->isSuppressSecond();
                          }, [](const bool& v, const LC_DimStyle* ds) -> void {
                              ds->extensionLine()->setSuppressSecond(!v);
                          }, list, cont);
        }
    }
    else {
        addLineWidth_DS({"dimExtLineWeight", tr("Ext line lineweight"), tr("Specifies lineweight for extension line (DIMLWE variable)")},
                        [](const LC_DimStyle* ds) -> RS2::LineWidth {
                            return ds->extensionLine()->lineWidth();
                        }, [](const RS2::LineWidth& l, const LC_DimStyle* ds) -> void {
                            ds->extensionLine()->setLineWidth(l);
                        }, list, cont);
        addLineType_DS({"dimExtLineType", tr("Ext line linetype"), tr("Specifies the linetype for the extension line (DIMLTEX1 variable)")},
                       [](const LC_DimStyle* ds) -> RS2::LineType {
                           return ds->extensionLine()->lineTypeFirst();
                       }, [](const RS2::LineType& linetype, const LC_DimStyle* ds) -> void {
                           const auto line = ds->extensionLine();
                           line->setLineTypeFirst(linetype);
                       }, list, cont);
    }
    if (!radialOrDiametric) {
        addBoolean_DS({"dimExtFixed", tr("Ext line fixed"), tr("Sets suppression of extension line fixed length (DIMFXLON variable)")},
                      [](const LC_DimStyle* ds) -> bool {
                          return ds->extensionLine()->hasFixedLength();
                      }, [](const bool& v, const LC_DimStyle* ds) -> void {
                          ds->extensionLine()->setHasFixedLength(v);
                      }, list, cont);

        addDouble_DS({"dimExtFixedLen", tr("Ext line fixed length"), tr("Sets extension line fixed length (DIMFXL variable)")},
                     [](const LC_DimStyle* ds) -> double {
                         return ds->extensionLine()->fixedLength();
                     }, [](const double& v, const LC_DimStyle* ds) -> void {
                         ds->extensionLine()->setFixedLength(v);
                     }, list, cont);
    }

    addColor_DS({"dimExtLineColor", tr("Ext line color"), tr("Specifies color of the extension line (DIMCLRE variable)")},
                [](const LC_DimStyle* ds) -> RS_Color {
                    const auto line = ds->extensionLine();
                    return line->color();
                }, [](const RS_Color& color, const LC_DimStyle* ds) -> void {
                    const auto line = ds->extensionLine();
                    line->setColor(color);
                }, list, cont);

    if (notDimOrdinate) {
        addDouble_DS({
                         "dimExtExtent",
                         tr("Ext line ext"),
                         tr("Specifies amount to extend extension line beyond the dimension line  (DIMEXE variable)")
                     }, [](const LC_DimStyle* ds) -> double {
                         return ds->extensionLine()->distanceBeyondDimLine();
                     }, [](const double& v, const LC_DimStyle* ds) -> void {
                         ds->extensionLine()->setDistanceBeyondDimLine(v);
                     }, list, cont, funPositiveDoubleSpin);
    }

    addDouble_DS(
        {"dimExtOffset", tr("Ext line offset"), tr("Specifies offset of extension lines from the origin points (DIMEXO variable)")},
        [](const LC_DimStyle* ds) -> double {
            return ds->extensionLine()->distanceFromOriginPoint();
        }, [](const double& v, const LC_DimStyle* ds) -> void {
            ds->extensionLine()->setDistanceFromOriginPoint(v);
        }, list, cont, funPositiveDoubleSpin);
}

void LC_PropertiesProviderDimBase::createTextSection(LC_PropertyContainer* container, const QList<RS_Entity*>& list) {
    const auto cont = createSection(container, {SECTION_DIM_TEXT, tr("Text"), tr("Text settings for dimensions")});

    addColor_DS({"dimTextFillColor", tr("Fill color"), tr("Specifies the background color of the dimension (DIMTFILL variable)")},
                [](const LC_DimStyle* ds) -> RS_Color {
                    const auto text = ds->text();
                    return text->explicitBackgroundFillColor();
                }, [](const RS_Color& color, const LC_DimStyle* ds) -> void {
                    const auto text = ds->text();
                    text->setExplicitBackgroundFillColor(color);
                }, list, cont);

    addColor_DS({"dimTextColor", tr("Text color"), tr("Specifies the color of the dimension of the text (DIMCLRT variable)")},
                [](const LC_DimStyle* ds) -> RS_Color {
                    const auto text = ds->text();
                    return text->color();
                }, [](const RS_Color& color, const LC_DimStyle* ds) -> void {
                    const auto text = ds->text();
                    text->setColor(color);
                }, list, cont);

    auto funPositiveDoubleSpin = []([[maybe_unused]] RS_Dimension* dimension, LC_PropertyViewDescriptor& descriptor)-> bool {
        descriptor.attributes[LC_PropertyDoubleSpinBoxView::ATTR_MIN] = 0.0;
        descriptor.attributes[LC_PropertyDoubleSpinBoxView::ATTR_STEP] = 0.01;
        return false;
    };

    addDouble_DS({"dimTextHeight", tr("Text height"), tr("Specifies text height of dimension (DIMTXT variable)")},
                 [](const LC_DimStyle* ds) -> double {
                     return ds->text()->height();
                 }, [](const double& v, const LC_DimStyle* ds) -> void {
                     ds->text()->setHeight(v);
                 }, list, cont, funPositiveDoubleSpin);

    addDouble_DS({
                     "dimTextOffset",
                     tr("Text offset"),
                     tr("Specifies the distance around dimension text when dimension line breaks for dimension text (DIMGAP variable)")
                 }, [](const LC_DimStyle* ds) -> double {
                     return ds->dimensionLine()->lineGap();
                 }, [](const double& v, const LC_DimStyle* ds) -> void {
                     ds->dimensionLine()->setLineGap(v);
                 }, list, cont, funPositiveDoubleSpin);

    if (m_entityType != RS2::EntityDimOrdinate) {
        addBoolean_DS({
                          "dimTextOrientationOutside",
                          tr("Text outside align"),
                          tr("Sets positioning of dimension text outside of extension lines (DIMTOH variable)")
                      }, [](const LC_DimStyle* ds) -> bool {
                          return ds->text()->isAlignedIfOutside();
                      }, [](const bool& v, const LC_DimStyle* ds) -> void {
                          ds->text()->setAlignedIfOutside(v);
                      }, list, cont);
    }

    if (m_entityType != RS2::EntityDimOrdinate && m_entityType != RS2::EntityDimRadial && m_entityType != RS2::EntityDimDiametric) {
        static LC_EnumDescriptor dimjustPolicyDescriptor = {
            "DIMJUSTPolicy",
            {
                {LC_DimStyle::Text::ABOVE_AND_CENTERED, tr("Centered")},
                {LC_DimStyle::Text::NEXT_TO_EXT_ONE, tr("First ext line")},
                {LC_DimStyle::Text::NEXT_TO_EXT_TWO, tr("Second ext line")},
                {LC_DimStyle::Text::ABOVE_ALIGN_EXT_ONE, tr("Over first ext line")},
                {LC_DimStyle::Text::ABOVE_ALIGN_EXT_TWO, tr("Over second ext line")}
            }
        };
        addEnum_DS({"dimTextHor", tr("Text pos hor"), tr("Specified horizontal dimension text position (DIMJUST variable)")},
                   &dimjustPolicyDescriptor, [](const LC_DimStyle* ds) -> LC_PropertyEnumValueType {
                       return ds->text()->horizontalPositioning();
                   }, [](const LC_PropertyEnumValueType& v, const LC_DimStyle* ds) -> void {
                       ds->text()->setHorizontalPositioningRaw(v);
                   }, list, cont);
    }
    static LC_EnumDescriptor dimtadPolicyDescriptor = {
        "DIMTADPolicy",
        {
            {LC_DimStyle::Text::CENTER_BETWEEN_EXT_LINES, tr("Centered")},
            {LC_DimStyle::Text::ABOVE_DIM_LINE_EXCEPT_NOT_HORIZONTAL, tr("Above")},
            {LC_DimStyle::Text::FAREST_SIDE_FROM_DEF_POINTS, tr("Outside")},
            {LC_DimStyle::Text::JIS_POSTION, tr("JIS")},
            {LC_DimStyle::Text::BELOW_DIMENSION_LINE, tr("Below")}
        }
    };

    addEnum_DS({"dimTextVert", tr("Text pos vert"), tr("Specified vertical dimension text position (DIMTAD variable)")},
               &dimtadPolicyDescriptor, [](const LC_DimStyle* ds) -> LC_PropertyEnumValueType {
                   return ds->text()->verticalPositioning();
               }, [](const LC_PropertyEnumValueType& v, const LC_DimStyle* ds) -> void {
                   ds->text()->setVerticalPositioningRaw(v);
               }, list, cont);

    // fixme - add support of text style instead of font name
    addStringFont<RS_Dimension>({"dimTxtStyle", tr("Text style"), tr("Specifies style of text for dimension (DIMTXSTY variable)")},
                                [](RS_Dimension* e) -> QString {
                                    const LC_DimStyle* dimStyle = e->getEffectiveCachedDimStyle();
                                    return dimStyle->text()->style();
                                }, [](const QString& v, RS_Dimension* e) -> void {
                                    const LC_DimStyle* dimStyle = e->getEffectiveDimStyleOverride();
                                    dimStyle->text()->setStyle(v);
                                }, list, cont);

    if (m_entityType != RS2::EntityDimOrdinate) {
        addBoolean_DS({
                          "dimTextOrientationInside",
                          tr("Text inside align"),
                          tr("Sets positioning of dimension text inside of extension lines (DIMTIH variable)")
                      }, [](const LC_DimStyle* ds) -> bool {
                          return ds->text()->isAlignedIfInside();
                      }, [](const bool& v, const LC_DimStyle* ds) -> void {
                          ds->text()->setAlignedIfInside(v);
                      }, list, cont);
    }

    addVector<RS_Dimension>({"textPosition", tr("Text position"), tr("Specifies dimension text position")},
                            [](const RS_Dimension* e) -> RS_Vector {
                                return e->getMiddleOfText();
                            }, [](const RS_Vector& v, RS_Dimension* e) -> void {
                                e->setMiddleOfText(v);
                            }, list, cont);

    if (m_entityType == RS2::EntityDimArc) {
        static LC_EnumDescriptor dimarcsymPolicyDescriptor = {
            "DIMARCSYMPolicy",
            {
                {LC_DimStyle::Arc::BEFORE, tr("Preceeding dimension text")},
                {LC_DimStyle::Arc::ABOVE, tr("Above dimension text")},
                {LC_DimStyle::Arc::NONE, tr("None")}
            }
        };

        addEnum_DS({
                       "dimTextArcLen",
                       tr("Arc length symbol"),
                       tr("Specifies placement of the arch length dimension symbol (DIMARCSYM variable)")
                   }, &dimarcsymPolicyDescriptor, [](const LC_DimStyle* ds) -> LC_PropertyEnumValueType {
                       return ds->arc()->arcSymbolPosition();
                   }, [](const LC_PropertyEnumValueType& v, const LC_DimStyle* ds) -> void {
                       ds->arc()->setArcSymbolPositionRaw(v);
                   }, list, cont);
    }

    // fixme - check whether wcs or relative angle should be used
    if (m_entityType != RS2::EntityDimArc) {
        // todo - AutoCAD does not support rotation on dim arc, check why it is so
        addWCSAngle<RS_Dimension>({"textRotation", tr("Text rotation"), tr("Specifies the rotation angle for dimension text")},
                                  [](const RS_Dimension* e) -> double {
                                      return e->getTextAngle();
                                  }, [](const double& v, RS_Dimension* l) -> void {
                                      l->setTextAngle(v);
                                  }, list, cont);
    }

    static LC_EnumDescriptor dimtxtdirectionPolicyDescriptor = {
        "DIMTXTDIRECTIONPolicy",
        {{LC_DimStyle::Text::LEFT_TO_RIGHT, tr("Left-to-right")}, {LC_DimStyle::Text::RIGHT_TO_LEFT, tr("Right-to-left")}}
    };

    addEnum_DS({"dimTextDir", tr("Text view direction"), tr("Specifies the text direction (DIMTXTDIRECTION variable)")},
               &dimtxtdirectionPolicyDescriptor, [](const LC_DimStyle* ds) -> LC_PropertyEnumValueType {
                   return ds->text()->readingDirection();
               }, [](const LC_PropertyEnumValueType& v, const LC_DimStyle* ds) -> void {
                   ds->text()->setReadingDirectionRaw(v);
               }, list, cont);

    addReadOnlyString<RS_Dimension>({"dimTextMeasurement", tr("Text measurement"), tr("Specified dimension measurement value")},
                                    [this](const RS_Dimension* e)-> QString {
                                        const double measurement = e->getMeasurement();
                                        QString value;
                                        if (e->rtti() == RS2::EntityDimAngular) {
                                            value = formatRawAngle(measurement, RS2::AngleFormat::DegreesDecimal);
                                        }
                                        else {
                                            value = formatDouble(measurement);
                                        }
                                        return value;
                                    }, list, cont);

    addString<RS_Dimension>({
                                "dimTextOverride",
                                tr("Text override"),
                                tr("Specified the text string of dimension (overrides Measurement string)")
                            }, [](const RS_Dimension* e) -> QString {
                                return e->getText();
                            }, [](const QString& v, RS_Dimension* e) -> void {
                                e->setLabel(v);
                            }, list, cont, false);
}

void LC_PropertiesProviderDimBase::createFitSection(LC_PropertyContainer* container, const QList<RS_Entity*>& list) {
    const auto cont = createSection(container, {SECTION_DIM_FIT, tr("Fit"), tr("Fit settings")});
    const bool notOrdinateDimension = m_entityType != RS2::EntityDimOrdinate;
    if (notOrdinateDimension) {
        addBoolean_DS({
                          "dimLinOutside",
                          tr("Dim line forced"),
                          tr("Force drawing dimension line between extension lines,"
                              " even if text placed outside of extension lines (DIMTOFL variable)")
                      }, [](const LC_DimStyle* ds) -> bool {
                          return ds->dimensionLine()->isDrawLineIfArrowsOutside();
                      }, [](const bool& v, const LC_DimStyle* ds) -> void {
                          ds->dimensionLine()->setDrawLineIfArrowsOutside(v);
                      }, list, cont);

        if (m_entityType != RS2::EntityDimDiametric && m_entityType != RS2::EntityDimRadial) {
            addBoolean_DS({
                              "dimLineInside",
                              tr("Dim line inside"),
                              tr("Force displaying dimension line outside extension lines (DIMSOXD variable)")
                          }, [](const LC_DimStyle* ds) -> bool {
                              return !ds->arrowhead()->isSuppressArrows();
                          }, [](const bool& v, const LC_DimStyle* ds) -> void {
                              ds->arrowhead()->setSuppressArrows(!v);
                          }, list, cont);
        }
    }

    auto funPositiveDoubleSpin = [](RS_Dimension*, LC_PropertyViewDescriptor& descriptor)-> bool {
        descriptor.attributes[LC_PropertyDoubleSpinBoxView::ATTR_MIN] = 0.0;
        descriptor.attributes[LC_PropertyDoubleSpinBoxView::ATTR_STEP] = 0.01;
        return false;
    };

    addDouble_DS({
                     "dimScale",
                     tr("Dim scale overall"),
                     tr("Specifies the overall scale factor applied to properties, that "
                         "specify sizes, distances or offsets (DIMSCALE variable)")
                 }, [](const LC_DimStyle* ds) -> double {
                     return ds->scaling()->scale();
                 }, [](const double& v, const LC_DimStyle* ds) -> void {
                     ds->scaling()->setScale(v);
                 }, list, cont, funPositiveDoubleSpin);

    if (notOrdinateDimension) {
        static LC_EnumDescriptor fitPolicyDescriptor = {
            "fitPolicy",
            {{0, tr("Best Fit")}, {1, tr("Arrows")}, {2, tr("Text")}, {3, tr("Both")}, {4, tr("Always fit")}}
        };

        addEnum_DS({
                       "fit",
                       tr("Fit"),
                       tr("Determines what elements are moved to fit text and "
                           "arrowheads in space between extension lines (DIMATFIT variable)")
                   }, &fitPolicyDescriptor, [](const LC_DimStyle* ds) -> LC_PropertyEnumValueType {
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
                   }, [](const LC_PropertyEnumValueType& v, const LC_DimStyle* ds) -> void {
                       LC_DimStyle::Text* text = ds->text();

                       if (v == 0) {
                           text->setUnsufficientSpacePolicy(LC_DimStyle::Text::EITHER_TEXT_OR_ARROW);
                           text->setExtLinesRelativePlacement(LC_DimStyle::Text::PLACE_BETWEEN_IF_SUFFICIENT_ROOM);
                       }
                       else if (v == 1) {
                           text->setUnsufficientSpacePolicy(LC_DimStyle::Text::ARROW_FIRST_THEN_TEXT);
                           text->setExtLinesRelativePlacement(LC_DimStyle::Text::PLACE_BETWEEN_IF_SUFFICIENT_ROOM);
                       }
                       else if (v == 2) {
                           text->setUnsufficientSpacePolicy(LC_DimStyle::Text::TEXT_FIRST_THEN_ARROW);
                           text->setExtLinesRelativePlacement(LC_DimStyle::Text::PLACE_BETWEEN_IF_SUFFICIENT_ROOM);
                       }
                       else if (v == 3) {
                           text->setUnsufficientSpacePolicy(LC_DimStyle::Text::OUTSIDE_EXT_LINES);
                           text->setExtLinesRelativePlacement(LC_DimStyle::Text::PLACE_BETWEEN_IF_SUFFICIENT_ROOM);
                       }
                       else if (v == 4) {
                           text->setExtLinesRelativePlacement(LC_DimStyle::Text::PLACE_ALWAYS_INSIDE);
                       }
                   }, list, cont);

        static LC_EnumDescriptor dimtixDescriptor = {
            "dimtix",
            {
                {LC_DimStyle::Text::PLACE_BETWEEN_IF_SUFFICIENT_ROOM, tr("Place bettween if has room")},
                {LC_DimStyle::Text::PLACE_ALWAYS_INSIDE, tr("Always inside")}
            }
        };

        addEnum_DS({"textInside", tr("Text inside"), tr("Sets position of dimension text inside extension lines (DIMTIX variable)")},
                   &dimtixDescriptor, [](const LC_DimStyle* ds) -> LC_PropertyEnumValueType {
                       return ds->text()->extLinesRelativePlacement();
                   }, [](const LC_PropertyEnumValueType& v, const LC_DimStyle* ds) -> void {
                       ds->text()->setExtLinesRelativePlacementRaw(v);
                   }, list, cont);
    }

    static LC_EnumDescriptor textMovementDescriptor = {
        "textMovement",
        {
            {LC_DimStyle::Text::DIM_LINE_WITH_TEXT, tr("Keep dim line with text")},
            {LC_DimStyle::Text::ADDS_LEADER, tr("Move text, add leader")},
            {LC_DimStyle::Text::ALLOW_FREE_POSITIONING, tr("Move text, no leader")}
        }
    };

    addEnum_DS({
                   "textMovement",
                   tr("Text movement"),
                   tr("Specifies position of the text when it is moved, either manually or automatically (DIMTMOVE variable)")
               }, &textMovementDescriptor, [](const LC_DimStyle* ds) -> LC_PropertyEnumValueType {
                   const LC_DimStyle::Text* text = ds->text();
                   const auto policy = text->positionMovementPolicy();
                   return policy;
               }, [](LC_PropertyEnumValueType& v, const LC_DimStyle* ds) -> void {
                   LC_DimStyle::Text* text = ds->text();
                   const auto policy = static_cast<LC_DimStyle::Text::TextMovementPolicy>(v);
                   text->setPositionMovementPolicy(policy);
               }, list, cont);
}

void LC_PropertiesProviderDimBase::addDecimalPlacesField_DS(const LC_Property::Names& precisionNames,
                                                            const std::function<LC_PropertyEnumValueType(LC_DimStyle* ds)>& funGetValue,
                                                            const std::function<void (LC_PropertyEnumValueType& v, LC_DimStyle* ds)>&
                                                            funSetValue, const QList<RS_Entity*>& list, LC_PropertyContainer* cont,
                                                            const std::function<bool(RS_Dimension*, LC_PropertyViewDescriptor&)>&
                                                            funPrepareDescriptor) {
    auto funPrecisionProvider = [](RS_Dimension* dim) -> LC_EnumDescriptor* {
        const auto dimStyle = dim->getEffectiveCachedDimStyle();
        const auto linearFormat = dimStyle->linearFormat();
        const auto format = linearFormat->format();
        return LC_PropertyProviderUtils::getLinearUnitsEnumDescriptor(format);
    };

    addVarEnum_DS(precisionNames, funPrecisionProvider, funGetValue, funSetValue, list, cont, funPrepareDescriptor);
}

void LC_PropertiesProviderDimBase::addLinearUnitFormat_DS(const LC_Property::Names& names,
                                                          const std::function<LC_PropertyEnumValueType(LC_DimStyle* ds)>& funGetValue,
                                                          const std::function<void (LC_PropertyEnumValueType& v, LC_DimStyle* ds)>&
                                                          funSetValue, const QList<RS_Entity*>& list, LC_PropertyContainer* cont,
                                                          const std::function<bool(RS_Dimension*, LC_PropertyViewDescriptor&)>&
                                                          funPrepareDescriptor) {
    const auto decimalLinearUnitsDescriptor = LC_PropertyProviderUtils::getLinearUnitFormatEnumDescriptor();
    addEnum_DS(names, decimalLinearUnitsDescriptor, funGetValue, funSetValue, list, cont, funPrepareDescriptor);
}

void LC_PropertiesProviderDimBase::addAngleDecimalPlacesField_DS(const LC_Property::Names& precisionNames,
                                                                 const std::function<LC_PropertyEnumValueType(LC_DimStyle* ds)>&
                                                                 funGetValue,
                                                                 const std::function<void(LC_PropertyEnumValueType& v, LC_DimStyle* ds)>&
                                                                 funSetValue, const QList<RS_Entity*>& list, LC_PropertyContainer* cont) {
    auto funPrecisionProvider = [](RS_Dimension* dim) -> LC_EnumDescriptor* {
        const auto dimStyle = dim->getEffectiveCachedDimStyle();
        const auto angularFormat = dimStyle->angularFormat();
        const auto format = angularFormat->format();
        return LC_PropertyProviderUtils::getAngleUnitsEnumDescriptor(format);
    };

    addVarEnum_DS(precisionNames, funPrecisionProvider, funGetValue, funSetValue, list, cont);
}

void LC_PropertiesProviderDimBase::addAngleUnitFormat(const LC_Property::Names& names,
                                                      const std::function<LC_PropertyEnumValueType(LC_DimStyle* ds)>& funGetValue,
                                                      const std::function<void(LC_PropertyEnumValueType& v, LC_DimStyle* ds)>& funSetValue,
                                                      const QList<RS_Entity*>& list, LC_PropertyContainer* cont) {
    const auto angleFormatDescriptor = LC_PropertyProviderUtils::getAngleUnitFormatEnumDescriptor();
    addEnum_DS(names, angleFormatDescriptor, funGetValue, funSetValue, list, cont);
}

void LC_PropertiesProviderDimBase::createPrimaryUnitsSection(LC_PropertyContainer* container, const QList<RS_Entity*>& list) {
    const auto cont = createSection(container, {SECTION_DIM_PRIMARY_UNITS, tr("Primary Units"), tr("Primary units for dimensions")});
    const bool notAngularDimension = m_entityType != RS2::EntityDimAngular;

    static LC_EnumDescriptor decimalSeparatorDescriptor = {"decimalSeparator", {{43, tr(".")}, {44, tr(",")}}};
    addEnum_DS(
        {"decimalSeparator", tr("Decimal separator"), tr("Specifies the decimal separator for metric dimensions (DIMDSEP variable)")},
        &decimalSeparatorDescriptor, [](const LC_DimStyle* ds) -> LC_PropertyEnumValueType {
            const auto linearFormat = ds->linearFormat();
            const auto separator = linearFormat->decimalFormatSeparatorChar();
            return separator;
        }, [](const LC_PropertyEnumValueType& v, const LC_DimStyle* ds) -> void {
            const auto linearFormat = ds->linearFormat();
            if (v == 44) {
                linearFormat->setDecimalFormatSeparatorChar(',');
            }
            else {
                linearFormat->setDecimalFormatSeparatorChar('.');
            }
        }, list, cont);

    addString_DS({"dimPrimaryPrefix", tr("Dim prefix"), tr("Specifies the text prefix for the dimensions (DIMPOST variable)")},
                 [](const LC_DimStyle* ds) -> QString {
                     const auto linearFormat = ds->linearFormat();
                     LC_DimStyle::LinearFormat::TextPattern* pattern = linearFormat->getPrimaryPrefixOrSuffix();
                     return pattern->getPrefix();
                 }, [](const QString& v, const LC_DimStyle* ds) -> void {
                     const auto linearFormat = ds->linearFormat();
                     LC_DimStyle::LinearFormat::TextPattern* pattern = linearFormat->getPrimaryPrefixOrSuffix();
                     pattern->setPrefix(v);
                 }, list, cont, true);

    addString_DS({"dimPrimarySuffix", tr("Dim suffix"), tr("Specifies the text suffix for the dimensions (DIMPOST variable)")},
                 [](const LC_DimStyle* ds) -> QString {
                     const auto linearFormat = ds->linearFormat();
                     LC_DimStyle::LinearFormat::TextPattern* pattern = linearFormat->getPrimaryPrefixOrSuffix();
                     return pattern->getSuffix();
                 }, [](const QString& v, const LC_DimStyle* ds) -> void {
                     const auto linearFormat = ds->linearFormat();
                     LC_DimStyle::LinearFormat::TextPattern* pattern = linearFormat->getPrimaryPrefixOrSuffix();
                     pattern->setSuffix(v);
                 }, list, cont, true);

    auto funPositiveDoubleSpinMetric = [](RS_Dimension* dimension, LC_PropertyViewDescriptor& descriptor)-> bool {
        descriptor.attributes[LC_PropertyDoubleSpinBoxView::ATTR_MIN] = 0.0;
        descriptor.attributes[LC_PropertyDoubleSpinBoxView::ATTR_STEP] = 0.01;
        const auto dimStyle = dimension->getEffectiveCachedDimStyle();
        const bool readonly = dimStyle->linearFormat()->isPrimaryMetric();
        return readonly;
    };

    auto funPositiveDoubleSpinNonMetric = [](RS_Dimension* dimension, LC_PropertyViewDescriptor& descriptor)-> bool {
        descriptor.attributes[LC_PropertyDoubleSpinBoxView::ATTR_MIN] = 0.0;
        descriptor.attributes[LC_PropertyDoubleSpinBoxView::ATTR_STEP] = 0.01;
        const auto dimStyle = dimension->getEffectiveCachedDimStyle();
        const bool readonly = !dimStyle->linearFormat()->isPrimaryMetric();
        return readonly;
    };

    auto funPositiveDoubleSpin = []([[maybe_unused]] RS_Dimension* dimension, LC_PropertyViewDescriptor& descriptor)-> bool {
        descriptor.attributes[LC_PropertyDoubleSpinBoxView::ATTR_MIN] = 0.0;
        descriptor.attributes[LC_PropertyDoubleSpinBoxView::ATTR_STEP] = 0.01;
        return false;
    };

    if (notAngularDimension) {
        addDouble_DS({"dimPrimaryRoundoff", tr("Dim roundoff"), tr("Specifies the distance rounding value (DIMRND variable)")},
                     [](const LC_DimStyle* ds) -> double {
                         const auto roundoff = ds->roundOff();
                         return roundoff->roundTo();
                     }, [](const double& v, const LC_DimStyle* ds) -> void {
                         const auto roundoff = ds->roundOff();
                         roundoff->setRoundToValue(v);
                     }, list, cont, funPositiveDoubleSpin);

        addDouble_DS({
                         "dimPrimaryScalelinear",
                         tr("Dim scale linear"),
                         tr("Specifies global scale factor for linear dimensions (DIMLFAC variable)")
                     }, [](const LC_DimStyle* ds) -> double {
                         const auto scaling = ds->scaling();
                         return scaling->linearFactor();
                     }, [](const double& v, const LC_DimStyle* ds) -> void {
                         const auto scaling = ds->scaling();
                         scaling->setLinearFactor(v);
                     }, list, cont, funPositiveDoubleSpin);

        if (m_entityType != RS2::EntityDimDiametric && m_entityType != RS2::EntityDimRadial) {
            addDouble_DS({
                             "dimPrimarySubUnitsScale",
                             tr("Dim sub-units scale"),
                             tr("Specifies sub-units scale factor for all applicable linear dimensions")
                         }, [](const LC_DimStyle* ds) -> double {
                             auto suppression = ds->zerosSuppression();
                             return /*suppression->roundTo();*/ 0.0; // fixme - dims - where from it's obtained from dxf point of view?
                         }, [](double& /*v*/, const LC_DimStyle* /* ds*/) -> void {
                             // fixme - dims - where from it's obtained?
                         }, list, cont, funPositiveDoubleSpin);
        }

        addLinearUnitFormat_DS(
            {"dimPrimaryLinearUnits", tr("Dim units"), tr("Specifies units format for linear dimensions (DIMLUNIT variable)")},
            [](const LC_DimStyle* ds) -> LC_PropertyEnumValueType {
                const auto linearFormat = ds->linearFormat();
                const auto format = linearFormat->format();
                return format;
            }, [](LC_PropertyEnumValueType& v, const LC_DimStyle* ds) -> void {
                const auto linearFormat = ds->linearFormat();
                const auto format = static_cast<RS2::LinearFormat>(v);
                linearFormat->setFormat(format);
            }, list, cont);
    }

    auto funIsMetricLinearFormat = [](RS_Dimension* dim, [[maybe_unused]] LC_PropertyViewDescriptor& descriptor)-> bool {
        const auto dimStyle = dim->getEffectiveCachedDimStyle();
        const bool readonly = dimStyle->linearFormat()->isPrimaryMetric();
        return readonly;
    };

    auto funIsNonMetricLinearFormat = [](RS_Dimension* dim, [[maybe_unused]] LC_PropertyViewDescriptor& descriptor)-> bool {
        const auto dimStyle = dim->getEffectiveCachedDimStyle();
        const bool readonly = !dimStyle->linearFormat()->isPrimaryMetric();
        return readonly;
    };

    addBoolean_DS({
                      "dimPrimarySuppressLeading",
                      tr("Suppress leading zeros"),
                      tr("Sets suppression of leading zeros for dimensions (DIMZIN variable)")
                  }, [](const LC_DimStyle* ds) -> bool {
                      return ds->zerosSuppression()->isSuppressLeading();
                  }, [](const bool& v, const LC_DimStyle* ds) -> void {
                      ds->zerosSuppression()->setSuppressLeading(v);
                  }, list, cont, LC_PropertyBoolCheckBoxView::VIEW_NAME, funIsNonMetricLinearFormat);

    addBoolean_DS({
                      "dimPrimarySuppressTrailing",
                      tr("Suppress trailing zeros"),
                      tr("Sets suppression of trailing zeros for dimensions (DIMZIN variable)")
                  }, [](const LC_DimStyle* ds) -> bool {
                      return ds->zerosSuppression()->isSuppressTrailing();
                  }, [](const bool& v, const LC_DimStyle* ds) -> void {
                      ds->zerosSuppression()->setSuppressTrailing(v);
                  }, list, cont, LC_PropertyBoolCheckBoxView::VIEW_NAME, funIsNonMetricLinearFormat);

    if (notAngularDimension) {
        addBoolean_DS({
                          "dimPrimarySuppressZeroFeet",
                          tr("Suppress zero feet"),
                          tr("Sets suppression of zero feet in dimension (DIMZIN variable)")
                      }, [](const LC_DimStyle* ds) -> bool {
                          const auto zerosSuppression = ds->zerosSuppression();
                          return zerosSuppression->isSuppressZeroFeets();
                      }, [](const LC_PropertyEnumValueType& v, const LC_DimStyle* ds) -> void {
                          const auto zerosSuppression = ds->zerosSuppression();
                          zerosSuppression->setSuppressZeroFeets(v);
                      }, list, cont, LC_PropertyBoolCheckBoxView::VIEW_NAME, funIsMetricLinearFormat);

        addBoolean_DS({
                          "dimPrimarySuppressZeroInches",
                          tr("Suppress zero inches"),
                          tr("Sets suppression of zero inches for dimension (DIMZIN variable)")
                      }, [](const LC_DimStyle* ds) -> bool {
                          const auto zerosSuppression = ds->zerosSuppression();
                          return zerosSuppression->isSuppressZeroInches();
                      }, [](const LC_PropertyEnumValueType& v, const LC_DimStyle* ds) -> void {
                          const auto zerosSuppression = ds->zerosSuppression();
                          zerosSuppression->setSuppressZeroInches(v);
                      }, list, cont, LC_PropertyBoolCheckBoxView::VIEW_NAME, funIsMetricLinearFormat);

        addDecimalPlacesField_DS(
            {"dimPrimaryDecimalPlaces", tr("Precision"), tr("Specifies precision for primary units dimensions (DIMDEC variable)")},
            [](const LC_DimStyle* ds) -> LC_PropertyEnumValueType {
                const auto linearFormat = ds->linearFormat();
                const auto format = linearFormat->decimalPlaces();
                return format;
            }, [](const LC_PropertyEnumValueType& v, const LC_DimStyle* ds) -> void {
                const auto linearFormat = ds->linearFormat();
                linearFormat->setDecimalPlaces(v);
            }, list, cont);
    }
    else {
        addAngleDecimalPlacesField_DS({
                                          "dimPrimaryAngularPlaces",
                                          tr("Angle precision"),
                                          tr(
                                              "Specifies number of precision decimal places displayed for angular dimension text (DIMADEC variable)")
                                      }, [](const LC_DimStyle* ds) -> LC_PropertyEnumValueType {
                                          const auto linearFormat = ds->angularFormat();
                                          const auto format = linearFormat->decimalPlaces();
                                          return format;
                                      }, [](const LC_PropertyEnumValueType& v, const LC_DimStyle* ds) -> void {
                                          const auto linearFormat = ds->angularFormat();
                                          linearFormat->setDecimalPlaces(v);
                                      }, list, cont);

        addAngleUnitFormat({"dimPrimaryAngularFormat", tr("Angle format"), tr("Specifies the angle format (DIMAUNIT variable)")},
                           [](const LC_DimStyle* ds) -> LC_PropertyEnumValueType {
                               const auto angularFormat = ds->angularFormat();
                               const auto format = angularFormat->format();
                               return format;
                           }, [](LC_PropertyEnumValueType& v, const LC_DimStyle* ds) -> void {
                               const auto angularFormat = ds->angularFormat();
                               const auto format = static_cast<RS2::AngleFormat>(v);
                               angularFormat->setFormat(format);
                           }, list, cont);
    }
}

void LC_PropertiesProviderDimBase::createAlternateUnitsSection(LC_PropertyContainer* container, const QList<RS_Entity*>& list) {
    const auto cont = createSection(container, {SECTION_DIM_ALTERNATE_UNITS, tr("Alternate Units"), tr("Alternate units for dimensions")});

    addBoolean_DS({
                      "dimAltEnabled",
                      tr("Alt enabled"),
                      tr("Sets units format for alternate units dimensions On or Off except angular (DIMALT variable)")
                  }, [](LC_DimStyle* ds) -> bool {
                      return ds->hasAltUnits();
                  }, [](const bool& v, const LC_DimStyle* ds) -> void {
                      ds->linearFormat()->setAlternateUnits(v ? LC_DimStyle::LinearFormat::ENABLE : LC_DimStyle::LinearFormat::DISABLE);
                  }, list, cont);

    auto funAltUnitsDisabled = [](RS_Dimension* dim, [[maybe_unused]] LC_PropertyViewDescriptor& descriptor)-> bool {
        const auto dimStyle = dim->getEffectiveCachedDimStyle();
        const bool readonly = dimStyle->hasNoAltUnits();
        return readonly;
    };

    addLinearUnitFormat_DS({
                               "dimAltLinearUnits",
                               tr("Alt format"),
                               tr("Specifies units format for alternate units dimensions except angular (DIMALTU variable)")
                           }, [](const LC_DimStyle* ds) -> LC_PropertyEnumValueType {
                               const auto linearFormat = ds->linearFormat();
                               const auto format = linearFormat->altFormat();
                               return format;
                           }, [](LC_PropertyEnumValueType& v, const LC_DimStyle* ds) -> void {
                               const auto linearFormat = ds->linearFormat();
                               const auto format = static_cast<RS2::LinearFormat>(v);
                               linearFormat->setAltFormat(format);
                           }, list, cont, funAltUnitsDisabled);

    addDecimalPlacesField_DS({
                                 "dimAltDecimalPlaces",
                                 tr("Alt precision"),
                                 tr("Specifies precision for alternate units dimensions (DIMALTD variable)")
                             }, [](const LC_DimStyle* ds) -> LC_PropertyEnumValueType {
                                 const auto linearFormat = ds->linearFormat();
                                 const auto format = linearFormat->altDecimalPlaces();
                                 return format;
                             }, [](const LC_PropertyEnumValueType& v, const LC_DimStyle* ds) -> void {
                                 const auto linearFormat = ds->linearFormat();
                                 linearFormat->setAltDecimalPlaces(v);
                             }, list, cont, funAltUnitsDisabled);

    addDouble_DS({"dimAltRoundoff", tr("Alt round"), tr("Specifies distances rounding value for alternate units (DIMALTRND variable)")},
                 [](const LC_DimStyle* ds) -> double {
                     const auto roundoff = ds->roundOff();
                     return roundoff->altRoundTo();
                 }, [](const double& v, const LC_DimStyle* ds) -> void {
                     const auto roundoff = ds->roundOff();
                     roundoff->setAltRoundToValue(v);
                 }, list, cont, funAltUnitsDisabled);

    auto funPositiveDoubleSpin = [](RS_Dimension* dim, LC_PropertyViewDescriptor& descriptor)-> bool {
        descriptor.attributes[LC_PropertyDoubleSpinBoxView::ATTR_MIN] = 0.0;
        descriptor.attributes[LC_PropertyDoubleSpinBoxView::ATTR_STEP] = 0.01;
        const bool readonly = dim->getEffectiveCachedDimStyle()->hasNoAltUnits();
        return readonly;
    };

    addDouble_DS({"dimAltScaleLinear", tr("Alt scale factor"), tr("Specifies scale factor for alternative units (DIMALTF variable)")},
                 [](const LC_DimStyle* ds) -> double {
                     const auto linearFormat = ds->linearFormat();
                     return linearFormat->altUnitsMultiplier();
                 }, [](const double& v, const LC_DimStyle* ds) -> void {
                     const auto linearFormat = ds->linearFormat();
                     linearFormat->setAltUnitsMultiplier(v);
                 }, list, cont, funPositiveDoubleSpin);

    auto funIsMetricLinearFormat = [](RS_Dimension* dim, [[maybe_unused]] LC_PropertyViewDescriptor& descriptor)-> bool {
        const auto dimStyle = dim->getEffectiveCachedDimStyle();
        if (dimStyle->hasNoAltUnits()) {
            return true;
        }
        return dimStyle->linearFormat()->isAltMetric();
    };

    auto funIsNonMetricLinearFormat = [](RS_Dimension* dim, [[maybe_unused]] LC_PropertyViewDescriptor& descriptor)-> bool {
        const auto dimStyle = dim->getEffectiveCachedDimStyle();
        if (dimStyle->hasNoAltUnits()) {
            return true;
        }
        return !dimStyle->linearFormat()->isAltMetric();
    };

    addBoolean_DS({
                      "dimAltSuppressLeading",
                      tr("Alt suppress leading zeros"),
                      tr("Sets suppression of leading zeros for alternate units in dimension (DIMALTZ variable)")
                  }, [](const LC_DimStyle* ds) -> bool {
                      return ds->zerosSuppression()->isAltSuppressLeading();
                  }, [](const bool& v, const LC_DimStyle* ds) -> void {
                      ds->zerosSuppression()->setAltSuppressLeading(v);
                  }, list, cont, LC_PropertyBoolCheckBoxView::VIEW_NAME, funIsNonMetricLinearFormat);

    addBoolean_DS({
                      "dimAltSuppressTrailing",
                      tr("Alt suppress trailing zeros"),
                      tr("Sets suppression of trailing zeros for alternate units in dimension (DIMALTZ variable)")
                  }, [](const LC_DimStyle* ds) -> bool {
                      return ds->zerosSuppression()->isAltSuppressTrailing();
                  }, [](const bool& v, const LC_DimStyle* ds) -> void {
                      ds->zerosSuppression()->setAltSuppressTrailing(v);
                  }, list, cont, LC_PropertyBoolCheckBoxView::VIEW_NAME, funIsNonMetricLinearFormat);

    addBoolean_DS({
                      "dimAltSuppressZeroFeet",
                      tr("Alt suppress zero feet"),
                      tr("Sets suppression of zero feet for alternate units in dimension (DIMALTZ variable)")
                  }, [](const LC_DimStyle* ds) -> bool {
                      const auto zerosSuppression = ds->zerosSuppression();
                      return zerosSuppression->isAltSuppressZeroFeets();
                  }, [](const bool& v, const LC_DimStyle* ds) -> void {
                      ds->zerosSuppression()->setAltSuppressZeroFeets(v);
                  }, list, cont, LC_PropertyBoolCheckBoxView::VIEW_NAME, funIsMetricLinearFormat);

    addBoolean_DS({
                      "dimAltSuppressZeroInches",
                      tr("Alt suppress zero inches"),
                      tr("Sets suppression of zero inches for alternate units in dimension (DIMALTZ variable)")
                  }, [](const LC_DimStyle* ds) -> bool {
                      const auto zerosSuppression = ds->zerosSuppression();
                      return zerosSuppression->isAltSuppressZeroInches();
                  }, [](const bool& v, const LC_DimStyle* ds) -> void {
                      ds->zerosSuppression()->setAltSuppressZeroInches(v);
                  }, list, cont, LC_PropertyBoolCheckBoxView::VIEW_NAME, funIsMetricLinearFormat);

    addString_DS({"dimAltPrefix", tr("Alt prefix"), tr("Specifies text prefix to alternate dimensions except angular (DIMAPOST variable)")},
                 [](const LC_DimStyle* ds) -> QString {
                     const auto linearFormat = ds->linearFormat();
                     LC_DimStyle::LinearFormat::TextPattern* pattern = linearFormat->getAlternativePrefixOrSuffix();
                     return pattern->getPrefix();
                 }, [](const QString& v, const LC_DimStyle* ds) -> void {
                     const auto linearFormat = ds->linearFormat();
                     LC_DimStyle::LinearFormat::TextPattern* pattern = linearFormat->getAlternativePrefixOrSuffix();
                     pattern->setPrefix(v);
                 }, list, cont, true, funAltUnitsDisabled);

    addString_DS({"dimAltSuffix", tr("Alt suffix"), tr("Specifies text suffix to alternate dimensions except angular (DIMAPOST variable)")},
                 [](const LC_DimStyle* ds) -> QString {
                     const auto linearFormat = ds->linearFormat();
                     LC_DimStyle::LinearFormat::TextPattern* pattern = linearFormat->getAlternativePrefixOrSuffix();
                     return pattern->getSuffix();
                 }, [](const QString& v, const LC_DimStyle* ds) -> void {
                     const auto linearFormat = ds->linearFormat();
                     LC_DimStyle::LinearFormat::TextPattern* pattern = linearFormat->getAlternativePrefixOrSuffix();
                     pattern->setSuffix(v);
                 }, list, cont, true, funAltUnitsDisabled);

    if (m_entityType != RS2::EntityDimDiametric && m_entityType != RS2::EntityDimRadial) {
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
        //                         }, nullptr/*[](double& v, RS_Dimension* e) -> void {
        //                         LC_DimStyle* dimStyle = e->getEffectiveCachedDimStyle();
        //                         auto roundoff = dimStyle->roundOff();
        //                         roundoff->setRoundToValue(v);
        //                         e->setDimStyleOverride(dimStyle);
        //                     }*/, list, cont, funPositiveDoubleSpin);
    }
}

void LC_PropertiesProviderDimBase::createTolerancesSection(LC_PropertyContainer* container, const QList<RS_Entity*>& list) {
    const auto cont = createSection(container, {SECTION_DIM_TOLERANCES, tr("Tolerances"), tr("Tolerance settings for dimensions")});

    static LC_EnumDescriptor dimtalntPolicyDescriptor = {
        "DIMTALNPolicy",
        {
            {LC_DimStyle::LatteralTolerance::ALIGN_OPERATIONAL_SYMBOLS, tr("Operational symbols")},
            {LC_DimStyle::LatteralTolerance::ALIGN_DECIMAL_SEPARATORS, tr("Decimal separators")}
        }
    };

    addEnum_DS({"dimTolAlign", tr("Tolerance alignment"), tr("Specifies alignment for stacked numbers (DIMTALN variable)")},
               &dimtalntPolicyDescriptor, [](const LC_DimStyle* ds) -> LC_PropertyEnumValueType {
                   return ds->latteralTolerance()->adjustment();
               }, [](const LC_PropertyEnumValueType& v, const LC_DimStyle* ds) -> void {
                   ds->latteralTolerance()->setAdjustmentRaw(v);
               }, list, cont);

    static LC_EnumDescriptor dimtolPolicyDescriptor = {
        "DIMTOLPolicy",
        {{0, tr("None")}, {1, tr("Symmetrical")}, {3, tr("Deviation")}, {4, tr("Limits")}, {5, tr("Basic")},}
    };

    addEnum_DS({
                   "dimTolDisplay",
                   tr("Tolerance display"),
                   tr("Specifies display mode of dimension tolerances to dimension text (DIMTOL variable)")
               }, &dimtolPolicyDescriptor, [](const LC_DimStyle* ds) -> LC_PropertyEnumValueType {
                   const auto tolerance = ds->latteralTolerance();
                   bool enable = false, showVerticalPosition = false, showLowerLimit = false, showUpperLimit = false;
                   const int tolMethod = LC_DlgDimStyleManager::computeToleranceMethod(ds, tolerance, enable, showVerticalPosition,
                                                                                       showLowerLimit, showUpperLimit);
                   return tolMethod;
               }, [](const LC_PropertyEnumValueType& v, const LC_DimStyle* ds) -> void {
                   bool enable = true;
                   const auto tol = ds->latteralTolerance();
                   const auto dimline = ds->dimensionLine();
                   bool showLowerLimit = true;
                   bool showVerticalPosition = false;
                   bool additionallyHideToleranceAdjustment = false;
                   bool drawFrame = false;
                   LC_DlgDimStyleManager::applyToleranceMethod(tol, dimline, v, enable, showLowerLimit, showVerticalPosition,
                                                               additionallyHideToleranceAdjustment, drawFrame);
               }, list, cont);

    addDouble_DS({
                     "dimTolLimitLower",
                     tr("Tolerance limit lower"),
                     tr("Specifies the minimal (or lower) tolerance limit for dimension text when DIMTOL or DIMLIM is on (DIMTM variable)")
                 }, [](const LC_DimStyle* ds) -> double {
                     return ds->latteralTolerance()->lowerToleranceLimit();
                 }, [](const double& v, const LC_DimStyle* ds) -> void {
                     ds->latteralTolerance()->setLowerToleranceLimit(v);
                 }, list, cont, [](RS_Dimension* dimension, LC_PropertyViewDescriptor& descriptor)-> bool {
                     descriptor.attributes[LC_PropertyDoubleSpinBoxView::ATTR_MIN] = 0.0;
                     descriptor.attributes[LC_PropertyDoubleSpinBoxView::ATTR_STEP] = 0.01;
                     bool enable = false, showVerticalPosition = false, showLowerLimit = false, showUpperLimit = false;
                     const LC_DimStyle* dimStyle = dimension->getEffectiveCachedDimStyle();
                     const auto tolerance = dimStyle->latteralTolerance();
                     [[maybe_unused]] int tolMethod = LC_DlgDimStyleManager::computeToleranceMethod(
                         dimStyle, tolerance, enable, showVerticalPosition, showLowerLimit, showUpperLimit);
                     return showLowerLimit;
                 });

    addDouble_DS({
                     "dimTolLimitUpper",
                     tr("Tolerance limit upper"),
                     tr("Specifies the maximum (or upper) tolerance limit for dimension text when DIMTOL or DIMLIM is on (DIMTP variable)")
                 }, [](const LC_DimStyle* ds) -> double {
                     return ds->latteralTolerance()->upperToleranceLimit();
                 }, [](const double& v, const LC_DimStyle* ds) -> void {
                     ds->latteralTolerance()->setUpperToleranceLimit(v);
                 }, list, cont, [](RS_Dimension* dimension, LC_PropertyViewDescriptor& descriptor)-> bool {
                     descriptor.attributes[LC_PropertyDoubleSpinBoxView::ATTR_MIN] = 0.0;
                     descriptor.attributes[LC_PropertyDoubleSpinBoxView::ATTR_STEP] = 0.01;
                     bool enable = false, showVerticalPosition = false, showLowerLimit = false, showUpperLimit = false;
                     const LC_DimStyle* dimStyle = dimension->getEffectiveCachedDimStyle();
                     const auto tolerance = dimStyle->latteralTolerance();
                     [[maybe_unused]] int tolMethod = LC_DlgDimStyleManager::computeToleranceMethod(
                         dimStyle, tolerance, enable, showVerticalPosition, showLowerLimit, showUpperLimit);
                     return showUpperLimit;
                 });

    static LC_EnumDescriptor dimtoljPolicyDescriptor = {
        "DIMTOLJPolicy",
        {
            {LC_DimStyle::LatteralTolerance::VerticalJustificationToDimText::BOTTOM, tr("Bottom")},
            {LC_DimStyle::LatteralTolerance::VerticalJustificationToDimText::MIDDLE, tr("Middle")},
            {LC_DimStyle::LatteralTolerance::VerticalJustificationToDimText::TOP, tr("Top")}
        }
    };

    addEnum_DS({
                   "dimTolPosVert",
                   tr("Tolerance pos vert"),
                   tr("Specifies vertical justification for tolerance values relative to nominal dimension text (DIMTOLJ variable)")
               }, &dimtoljPolicyDescriptor, [](const LC_DimStyle* ds) -> LC_PropertyEnumValueType {
                   const auto tolerance = ds->latteralTolerance();
                   return tolerance->verticalJustification();
               }, [](const LC_PropertyEnumValueType& v, const LC_DimStyle* ds) -> void {
                   const auto tol = ds->latteralTolerance();
                   tol->setVerticalJustificationRaw(v);
               }, list, cont);

    const bool notAngularDimension = m_entityType != RS2::EntityDimAngular;
    if (notAngularDimension) {
        addDecimalPlacesField_DS({
                                     "dimTolPrecision",
                                     tr("Tolerance precision"),
                                     tr("Specifies number of decimal places for tolerance values of a dimension (DIMTDEC variable)")
                                 }, [](const LC_DimStyle* ds) -> LC_PropertyEnumValueType {
                                     const auto tol = ds->latteralTolerance();
                                     const auto places = tol->decimalPlaces();
                                     return places;
                                 }, [](const LC_PropertyEnumValueType& v, const LC_DimStyle* ds) -> void {
                                     const auto tol = ds->latteralTolerance();
                                     tol->setDecimalPlaces(v);
                                 }, list, cont);
    }
    else {
        addAngleDecimalPlacesField_DS(
            {
                "dimTolAngleDecimalPlaces",
                tr("Tolerance precision"),
                tr("Specifies number of decimal places for tolerance values of a dimension (DIMTDEC variable)")
            }, [](const LC_DimStyle* ds) -> LC_PropertyEnumValueType {
                const auto tol = ds->latteralTolerance();
                const auto format = tol->decimalPlaces();
                return format;
            }, [](const LC_PropertyEnumValueType& v, const LC_DimStyle* ds) -> void {
                const auto tol = ds->latteralTolerance();
                tol->setDecimalPlaces(v);
            }, list, cont);
    }

    auto funIsMetricLinearFormat = [](RS_Dimension* dim, [[maybe_unused]] LC_PropertyViewDescriptor& descriptor)-> bool {
        const auto dimStyle = dim->getEffectiveCachedDimStyle();
        const bool readonly = dimStyle->linearFormat()->isPrimaryMetric();
        return readonly;
    };

    auto funIsNonMetricLinearFormat = [](RS_Dimension* dim, [[maybe_unused]] LC_PropertyViewDescriptor& descriptor)-> bool {
        const auto dimStyle = dim->getEffectiveCachedDimStyle();
        const bool readonly = !dimStyle->linearFormat()->isPrimaryMetric();
        return readonly;
    };

    addBoolean_DS({
                      "dimTolSuppressLeading",
                      tr("Tolerance suppress leading zeros"),
                      tr("Sets suppression of leading zeros for tolerance values in dimension (DIMTZIN value)")
                  }, [](const LC_DimStyle* ds) -> bool {
                      return ds->zerosSuppression()->isTolSuppressLeading();
                  }, [](const bool& v, const LC_DimStyle* ds) -> void {
                      ds->zerosSuppression()->setTolSuppressLeading(v);
                  }, list, cont, LC_PropertyBoolCheckBoxView::VIEW_NAME, funIsNonMetricLinearFormat);

    addBoolean_DS({
                      "dimTolSuppressTrailing",
                      tr("Tolerance suppress trailing zeros"),
                      tr("Sets suppression of trailing zeros for tolerance values in dimension (DIMTZIN value)")
                  }, [](const LC_DimStyle* ds) -> bool {
                      return ds->zerosSuppression()->isTolSuppressTrailing();
                  }, [](const bool& v, const LC_DimStyle* ds) -> void {
                      ds->zerosSuppression()->setTolSuppressTrailing(v);
                  }, list, cont, LC_PropertyBoolCheckBoxView::VIEW_NAME, funIsNonMetricLinearFormat);

    if (notAngularDimension) {
        addBoolean_DS({
                          "dimTolSuppressZeroFeet",
                          tr("Tolerance suppress zero feet"),
                          tr("Sets suppression of zero feet for tolerance values in dimension (DIMTZIN variable)")
                      }, [](const LC_DimStyle* ds) -> bool {
                          const auto zerosSuppression = ds->zerosSuppression();
                          return zerosSuppression->isTolSuppressZeroFeets();
                      }, [](const bool& v, const LC_DimStyle* ds) -> void {
                          ds->zerosSuppression()->setTolSuppressZeroFeets(v);
                      }, list, cont, LC_PropertyBoolCheckBoxView::VIEW_NAME, funIsMetricLinearFormat);

        addBoolean_DS({
                          "dimTolSuppressZeroInch",
                          tr("Tolerance suppress zero inches"),
                          tr("Sets suppression of zero inches for tolerance values in dimension (DIMTZIN variable)")
                      }, [](const LC_DimStyle* ds) -> bool {
                          const auto zerosSuppression = ds->zerosSuppression();
                          return zerosSuppression->isTolSuppressZeroInches();
                      }, [](const bool& v, const LC_DimStyle* ds) -> void {
                          ds->zerosSuppression()->setTolSuppressZeroInches(v);
                      }, list, cont, LC_PropertyBoolCheckBoxView::VIEW_NAME, funIsMetricLinearFormat);
    }

    addDouble_DS({
                     "dimTolTextHeight",
                     tr("Tolerance text height"),
                     tr(
                         "Specifies scale factor for text height of tolerance values relative to dimension text height as set by DIMTXT (DIMTFAC variable)")
                 }, [](const LC_DimStyle* ds) -> double {
                     return ds->latteralTolerance()->heightScaleFactorToDimText();
                 }, [](const double& v, const LC_DimStyle* ds) -> void {
                     ds->latteralTolerance()->setHeightScaleFactorToDimText(v);
                 }, list, cont, []([[maybe_unused]] RS_Dimension* dimension, LC_PropertyViewDescriptor& descriptor)-> bool {
                     descriptor.attributes[LC_PropertyDoubleSpinBoxView::ATTR_MIN] = 0.0;
                     descriptor.attributes[LC_PropertyDoubleSpinBoxView::ATTR_STEP] = 0.01;
                     return false;
                 });

    if (notAngularDimension) {
        auto funIsAltMetricLinearFormat = [](RS_Dimension* dim, [[maybe_unused]] LC_PropertyViewDescriptor& descriptor)-> bool {
            const auto dimStyle = dim->getEffectiveCachedDimStyle();
            const bool readonly = dimStyle->linearFormat()->isAltMetric() || dimStyle->hasNoAltUnits();
            return readonly;
        };

        auto funIsAltNonMetricLinearFormat = [](RS_Dimension* dim, [[maybe_unused]] LC_PropertyViewDescriptor& descriptor)-> bool {
            const auto dimStyle = dim->getEffectiveCachedDimStyle();
            const bool readonly = !dimStyle->linearFormat()->isAltMetric() || dimStyle->hasNoAltUnits();
            return readonly;
        };

        addDecimalPlacesField_DS({
                                     "dimTolAltPrecision",
                                     tr("Alt tolerance precision"),
                                     tr(
                                         "Specifies number of decimal places for tolerance values of an alternate units dimension (DIMALTTD variable)")
                                 }, [](const LC_DimStyle* ds) -> LC_PropertyEnumValueType {
                                     const auto tol = ds->latteralTolerance();
                                     const auto places = tol->decimalPlacesAltDim();
                                     return places;
                                 }, [](const LC_PropertyEnumValueType& v, const LC_DimStyle* ds) -> void {
                                     const auto tol = ds->latteralTolerance();
                                     tol->setDecimalPlacesAltDim(v);
                                 }, list, cont, [](RS_Dimension* dim, [[maybe_unused]] LC_PropertyViewDescriptor& descriptor)-> bool {
                                     const auto dimStyle = dim->getEffectiveCachedDimStyle();
                                     const bool readonly = dimStyle->hasNoAltUnits();
                                     return readonly;
                                 });

        addBoolean_DS({
                          "dimTolAltSuppressLeading",
                          tr("Alt tolerance suppress leading zeros"),
                          tr("Sets suppression of leading zeros for alternate units tolerance values in dimension (DIMALTTZ value)")
                      }, [](const LC_DimStyle* ds) -> bool {
                          return ds->zerosSuppression()->isAltTolSuppressLeading();
                      }, [](const bool& v, const LC_DimStyle* ds) -> void {
                          ds->zerosSuppression()->setAltTolSuppressLeading(v);
                      }, list, cont, LC_PropertyBoolCheckBoxView::VIEW_NAME, funIsAltNonMetricLinearFormat);

        addBoolean_DS({
                          "dimTolAltSuppressTrailing",
                          tr("Alt tolerance suppress trailing zeros"),
                          tr("Sets suppression of trailing zeros for alternate units tolerance values in dimension (DIMALTTZ value)")
                      }, [](const LC_DimStyle* ds) -> bool {
                          return ds->zerosSuppression()->isAltTolSuppressTrailing();
                      }, [](const bool& v, const LC_DimStyle* ds) -> void {
                          ds->zerosSuppression()->setAltTolSuppressTrailing(v);
                      }, list, cont, LC_PropertyBoolCheckBoxView::VIEW_NAME, funIsAltNonMetricLinearFormat);

        addBoolean_DS({
                          "dimTolAltSuppressZeroFeet",
                          tr("Alt tolerance suppress zero feet"),
                          tr("Sets suppression of zero feet for alternate units tolerance values in dimension (DIMALTTZ variable)")
                      }, [](const LC_DimStyle* ds) -> bool {
                          const auto zerosSuppression = ds->zerosSuppression();
                          return zerosSuppression->isAltTolSuppressZeroFeets();
                      }, [](const bool& v, const LC_DimStyle* ds) -> void {
                          ds->zerosSuppression()->setAltTolSuppressZeroFeets(v);
                      }, list, cont, LC_PropertyBoolCheckBoxView::VIEW_NAME, funIsAltMetricLinearFormat);

        addBoolean_DS({
                          "dimTolAltSuppressZeroInch",
                          tr("Alt tolerance suppress zero inches"),
                          tr("Sets suppression of zero inches for alterate units tolerance values in dimension (DIMALTTZ variable)")
                      }, [](const LC_DimStyle* ds) -> bool {
                          const auto zerosSuppression = ds->zerosSuppression();
                          return zerosSuppression->isAltTolSuppressZeroInches();
                      }, [](const bool& v, const LC_DimStyle* ds) -> void {
                          ds->zerosSuppression()->setAltTolSuppressZeroInches(v);
                      }, list, cont, LC_PropertyBoolCheckBoxView::VIEW_NAME, funIsAltMetricLinearFormat);
    }
}

void LC_PropertiesProviderDimBase::addColor_DS(const LC_Property::Names& names, const std::function<RS_Color(LC_DimStyle* ds)>& funGetValue,
                                               const std::function<void(RS_Color& v, LC_DimStyle* ds)>& funSetValue,
                                               const QList<RS_Entity*>& list, LC_PropertyContainer* cont) {
    add<RS_Dimension>(names, [this, funGetValue, funSetValue](const LC_Property::Names& n, RS_Dimension* entity,
                                                              LC_PropertyContainer* container, QList<LC_PropertyAtomic*>* props) -> void {
        auto* property = new LC_PropertyRSColor(container, false);
        property->setNames(n);
        createDelegatedStorage<RS_Color, RS_Dimension>([funGetValue](RS_Dimension* e) -> RS_Color {
                                                           LC_DimStyle* dimStyle = e->getEffectiveCachedDimStyle();
                                                           return funGetValue(dimStyle);
                                                       }, [funSetValue](RS_Color& v, RS_Dimension* e)-> void {
                                                           LC_DimStyle* dimStyle = e->getEffectiveDimStyleOverride();
                                                           funSetValue(v, dimStyle);
                                                       }, [funGetValue](const RS_Color& l, RS_Dimension* e) -> bool {
                                                           LC_DimStyle* dimStyle = e->getEffectiveCachedDimStyle();
                                                           const bool equals = l == funGetValue(dimStyle);
                                                           return equals;
                                                       }, entity, property);
        if (funGetValue == nullptr) {
            property->setReadOnly();
        }
        props->push_back(property);
    }, list, cont);
}

void LC_PropertiesProviderDimBase::addLineType_DS(const LC_Property::Names& names,
                                                  const std::function<RS2::LineType(LC_DimStyle* ds)>& funGetValue,
                                                  const std::function<void(RS2::LineType& v, LC_DimStyle* ds)>& funSetValue,
                                                  const QList<RS_Entity*>& list, LC_PropertyContainer* cont) {
    add<RS_Dimension>(names, [this, funGetValue, funSetValue](const LC_Property::Names& n, RS_Dimension* entity,
                                                              LC_PropertyContainer* container, QList<LC_PropertyAtomic*>* props) -> void {
        auto* property = new LC_PropertyLineType(container, false);
        property->setNames(n);
        createDelegatedStorage<RS2::LineType, RS_Dimension>([funGetValue](RS_Dimension* e)-> RS2::LineType {
                                                                LC_DimStyle* dimStyle = e->getEffectiveCachedDimStyle();
                                                                return funGetValue(dimStyle);
                                                            }, [funSetValue](RS2::LineType& l, RS_Dimension* e) -> void {
                                                                LC_DimStyle* dimStyle = e->getEffectiveDimStyleOverride();
                                                                funSetValue(l, dimStyle);
                                                            }, [funGetValue](const RS2::LineType& l, RS_Dimension* e) -> bool {
                                                                LC_DimStyle* ds = e->getEffectiveCachedDimStyle();
                                                                const bool equals = l == funGetValue(ds);
                                                                return equals;
                                                            }, entity, property);
        if (funGetValue == nullptr) {
            property->setReadOnly();
        }
        props->push_back(property);
    }, list, cont);
}

void LC_PropertiesProviderDimBase::addLineWidth_DS(const LC_Property::Names& names,
                                                   const std::function<RS2::LineWidth(LC_DimStyle* e)>& funGetValue,
                                                   const std::function<void(RS2::LineWidth& v, LC_DimStyle* e)>& funSetValue,
                                                   const QList<RS_Entity*>& list, LC_PropertyContainer* cont) {
    add<RS_Dimension>(names, [this, funGetValue, funSetValue](const LC_Property::Names& n, RS_Dimension* entity,
                                                              LC_PropertyContainer* container, QList<LC_PropertyAtomic*>* props) -> void {
        auto* property = new LC_PropertyLineWidth(container, false);
        property->setNames(n);
        createDelegatedStorage<RS2::LineWidth, RS_Dimension>([funGetValue](RS_Dimension* e) -> RS2::LineWidth {
                                                                 LC_DimStyle* dimStyle = e->getEffectiveCachedDimStyle();
                                                                 return funGetValue(dimStyle);
                                                             }, [funSetValue](RS2::LineWidth& l, RS_Dimension* e) -> void {
                                                                 LC_DimStyle* dimStyle = e->getEffectiveDimStyleOverride();
                                                                 funSetValue(l, dimStyle);
                                                             }, [funGetValue](const RS2::LineWidth& l, RS_Dimension* e) -> bool {
                                                                 LC_DimStyle* dimStyle = e->getEffectiveCachedDimStyle();
                                                                 const bool equals = l == funGetValue(dimStyle);
                                                                 return equals;
                                                             }, entity, property);
        if (funGetValue == nullptr) {
            property->setReadOnly();
        }
        props->push_back(property);
    }, list, cont);
}

void LC_PropertiesProviderDimBase::addBoolean_DS(const LC_Property::Names& names, const std::function<bool(LC_DimStyle* e)>& funGetValue,
                                                 const std::function<void(bool& v, LC_DimStyle* e)>& funSetValue,
                                                 const QList<RS_Entity*>& list, LC_PropertyContainer* container, const QString& viewName,
                                                 const std::function<bool(RS_Dimension*, LC_PropertyViewDescriptor& descriptor)>&
                                                 funPrepareDescriptor) {
    addBoolean<RS_Dimension>(names, [funGetValue](RS_Dimension* e) -> bool {
                                 LC_DimStyle* dimStyle = e->getEffectiveCachedDimStyle();
                                 return funGetValue(dimStyle);
                             }, [funSetValue](bool& v, RS_Dimension* e) -> void {
                                 LC_DimStyle* dimStyle = e->getEffectiveDimStyleOverride();
                                 funSetValue(v, dimStyle);
                             }, list, container, viewName, funPrepareDescriptor);
}

void LC_PropertiesProviderDimBase::addString_DS(const LC_Property::Names& names, const std::function<QString(LC_DimStyle* e)>& funGetValue,
                                                const std::function<void(QString& v, LC_DimStyle* e)>& funSetValue,
                                                const QList<RS_Entity*>& list, LC_PropertyContainer* container, const bool multiLine,
                                                const std::function<bool(RS_Dimension*, LC_PropertyViewDescriptor& descriptor)>&
                                                funPrepareDescriptor) {
    addString<RS_Dimension>(names, [funGetValue](RS_Dimension* e) -> QString {
                                LC_DimStyle* dimStyle = e->getEffectiveCachedDimStyle();
                                return funGetValue(dimStyle);
                            }, [funSetValue](QString& v, RS_Dimension* e) -> void {
                                LC_DimStyle* dimStyle = e->getEffectiveDimStyleOverride();
                                funSetValue(v, dimStyle);
                            }, list, container, multiLine, funPrepareDescriptor);
}

void LC_PropertiesProviderDimBase::addDouble_DS(const LC_Property::Names& names, const std::function<double(LC_DimStyle* e)>& funGetValue,
                                                const std::function<void(double& v, LC_DimStyle* e)>& funSetValue,
                                                const QList<RS_Entity*>& list, LC_PropertyContainer* container,
                                                const std::function<bool(RS_Dimension*, LC_PropertyViewDescriptor& descriptor)>&
                                                funPrepareDescriptor) {
    addDouble<RS_Dimension>(names, [funGetValue](RS_Dimension* e) -> double {
                                LC_DimStyle* dimStyle = e->getEffectiveCachedDimStyle();
                                return funGetValue(dimStyle);
                            }, [funSetValue](double& v, RS_Dimension* e) -> void {
                                LC_DimStyle* dimStyle = e->getEffectiveDimStyleOverride();
                                funSetValue(v, dimStyle);
                            }, list, container, funPrepareDescriptor);
}

void LC_PropertiesProviderDimBase::addEnum_DS(const LC_Property::Names& names, const LC_EnumDescriptor* enumDescriptor,
                                              const std::function<LC_PropertyEnumValueType(LC_DimStyle* e)>& funGetValue,
                                              const std::function<void (LC_PropertyEnumValueType& v, LC_DimStyle* e)>& funSetValue,
                                              const QList<RS_Entity*>& list, LC_PropertyContainer* container,
                                              const std::function<bool(RS_Dimension*, LC_PropertyViewDescriptor& descriptor)>&
                                              funPrepareDescriptor) {
    addEnum<RS_Dimension>(names, enumDescriptor, [funGetValue](RS_Dimension* e) -> LC_PropertyEnumValueType {
                              LC_DimStyle* dimStyle = e->getEffectiveCachedDimStyle();
                              return funGetValue(dimStyle);
                          }, [funSetValue](LC_PropertyEnumValueType& v, RS_Dimension* e) -> void {
                              LC_DimStyle* dimStyle = e->getEffectiveDimStyleOverride();
                              funSetValue(v, dimStyle);
                          }, list, container, funPrepareDescriptor);
}

void LC_PropertiesProviderDimBase::addVarEnum_DS(const LC_Property::Names& names,
                                                 const std::function<LC_EnumDescriptor *(RS_Dimension* dim)>& enumDescriptorProvider,
                                                 const std::function<LC_PropertyEnumValueType(LC_DimStyle* e)>& funGetValue,
                                                 const std::function<void (LC_PropertyEnumValueType& v, LC_DimStyle* e)>& funSetValue,
                                                 const QList<RS_Entity*>& list, LC_PropertyContainer* container,
                                                 const std::function<bool(RS_Dimension*, LC_PropertyViewDescriptor& descriptor)>&
                                                 funPrepareDescriptor) {
    addVarEnum<RS_Dimension>(names, enumDescriptorProvider, [funGetValue](RS_Dimension* e) -> LC_PropertyEnumValueType {
                                 LC_DimStyle* dimStyle = e->getEffectiveCachedDimStyle();
                                 return funGetValue(dimStyle);
                             }, [funSetValue](LC_PropertyEnumValueType& v, RS_Dimension* e) -> void {
                                 LC_DimStyle* dimStyle = e->getEffectiveDimStyleOverride();
                                 funSetValue(v, dimStyle);
                             }, list, container, funPrepareDescriptor);
}

void LC_PropertiesProviderDimBase::createDimGeometrySection(LC_PropertyContainer* container, const QList<RS_Entity*>& list) {
    const auto cont = createSection(container, {SECTION_DIM_GEOMETRY, tr("Dim geometry"), tr("Dimension's geometry properties")});
    doCreateDimGeometrySection(cont, list);
}

void LC_PropertiesProviderDimBase::doCreateSelectedSetCommands(LC_PropertyContainer* propertyContainer, const QList<RS_Entity*>& list) {
    LC_EntityTypePropertiesProvider::doCreateSelectedSetCommands(propertyContainer, list);
    const std::list<CommandLinkInfo> commands = {
        {
            tr("Regenerate dimensions"),
            {RS2::ActionDimRegenerate, tr("Dim Regenerate"), tr("Regenerate dimensions")},
            // {RS2::ActionDimeStyles, tr("Dimension Styles"), tr("Uses aligned dimension as base line and creates other dimensions")}
        }
    };
    createEntityContextCommands<RS_Dimension>(commands, propertyContainer, nullptr, "dimCommands", false);
}
