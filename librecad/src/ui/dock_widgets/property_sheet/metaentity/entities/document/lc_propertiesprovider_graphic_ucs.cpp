/*
 * ********************************************************************************
 * This file is part of the LibreCAD project, a 2D CAD program
 *
 * Copyright (C) 2026 LibreCAD.org
 * Copyright (C) 2026 sand1024
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

#include "lc_propertiesprovider_graphic_ucs.h"

#include "lc_propertyprovider_utils.h"
#include "lc_ucslistwidget.h"
#include "qc_applicationwindow.h"
#include "rs_graphic.h"

const QString LC_PropertiesProviderGraphicUCS::SECTION_UCS = "_secUCS";

using namespace LC_PropertyProviderUtils;

void LC_PropertiesProviderGraphicUCS::fillDocumentProperties(LC_PropertyContainer* container, RS_Graphic* graphic) {
    if (m_widget->getOptions()->noSelectionUCS) {
        const LC_Property::Names names = {SECTION_UCS, tr("User Coordinate System (UCS)"), tr("Active user coordinate system properties")};
        const auto cont = createSection(container, names);
        const auto ucsList = graphic->getUCSList();
        const auto currentUcs = graphic->getCurrentUCS();
        const QString currentUCSName = currentUcs->getName();

        const int currentUCSIndex = createUcsActive(graphic, ucsList, currentUcs, currentUCSName, cont);
        createUcsOriginAndDirection(cont, currentUcs);
        createUcsGrid(cont, currentUcs);
        const bool isUCS = currentUcs->isUCS();
        if (isShowLinks()) {
            createUCSListCommands(cont, graphic, ucsList, currentUCSIndex, isUCS);
            createUCSOperations(cont, graphic, ucsList, currentUCSIndex, isUCS);
        }

        delete currentUcs;
    }
}

int LC_PropertiesProviderGraphicUCS::createUcsActive(RS_Graphic* graphic, LC_UCSList* const ucsList, const LC_UCS* const currentUcs,
                                                     const QString& currentUCSName, LC_PropertyContainer* const cont) const {
    int currentUCSIndex = 0;
    QVector<LC_EnumValueDescriptor> values;
    const auto systemsList = ucsList->getItems();
    int i = 0;
    for (const auto u : *systemsList) {
        auto name = u->getName();
        if (currentUcs->isSameTo(u)) {
            if (name == currentUCSName) {
                currentUCSIndex = i;
            }
        }

        if (name.isEmpty()) {
            name = tr("<No name>");
        }
        auto desc = LC_EnumValueDescriptor(i, name);
        values.append(desc);
        i++;
    }
    const auto enumDescriptor = new LC_EnumDescriptor("ucsListEnum", values);

    auto funGet = [currentUCSIndex]([[maybe_unused]] const RS_Graphic* e)-> LC_PropertyEnumValueType {
        return currentUCSIndex;
    };

    auto funSet = [this,ucsList](const LC_PropertyEnumValueType& v, [[maybe_unused]] const RS_Graphic* e) -> void {
        LC_UCS* ucs = ucsList->at(v);
        if (ucs != nullptr) {
            const auto graphicViewport = m_actionContext->getViewport();
            if (graphicViewport != nullptr) {
                graphicViewport->applyUCS(ucs);
            }
        }
        m_widget->refill();
    };

    const LC_Property::Names names = {"activeUCS", tr("Active UCS"), tr("UCS switch to")};
    addDirectEnum<LC_PropertyEnumValueType, RS_Graphic>(cont, names, enumDescriptor, funGet, funSet, graphic, true);
    return currentUCSIndex;
}

void LC_PropertiesProviderGraphicUCS::createUcsGrid(LC_PropertyContainer* const cont, const LC_UCS* const currentUcs) {
    QString gridTypeStr;
    const auto isoGridViewType = currentUcs->getIsoGridViewType();
    switch (isoGridViewType) {
        case RS2::Ortho: {
            gridTypeStr = tr("Ortho");
            break;
        }
        case RS2::IsoLeft: {
            gridTypeStr = tr("Isometric Left");
            break;
        }
        case RS2::IsoRight: {
            gridTypeStr = tr("Isometric Right");
            break;
        }
        case RS2::IsoTop: {
            gridTypeStr = tr("Isometric Top");
            break;
        }
        default: {
            gridTypeStr = tr("Ortho");
            break;
        }
    }

    const LC_Property::Names names = {"ucsGridType", tr("Grid type"), tr("Type of grid used by coordinate system")};
    createDirectDelegatedReadonlyString(cont, names, gridTypeStr);
}

void LC_PropertiesProviderGraphicUCS::createUCSListCommands(LC_PropertyContainer* const cont, RS_Graphic* graphic,
                                                            LC_UCSList* const ucsList, int currentUCSIndex, const bool isUCS) const {
    auto clickHandler = [this,currentUCSIndex, ucsList]([[maybe_unused]] RS_Graphic* g, const int linkIndex) {
        switch (linkIndex) {
            case 0: {
                //adding ucs
                m_actionContext->setCurrentAction(RS2::ActionUCSCreate, nullptr);
                break;
            }
            case 1: {
                const auto existingUCS = ucsList->at(currentUCSIndex);
                if (existingUCS != nullptr) {
                    QC_ApplicationWindow::getAppWindow()->getUCSListWidget()->removeExistingUCS(existingUCS);
                }
                break;
            }
            default:
                break;
        }
    };

    createSingleEntityCommand<RS_Graphic>(cont, "ucsStructure", tr("Add UCS..."), tr("New UCS creation"),
        isUCS ? tr("Remove UCS...") : "",  isUCS ? tr("Invokes removal of current UCS") : "", graphic, clickHandler,
        tr(""));
}

void LC_PropertiesProviderGraphicUCS::createUCSOperations(LC_PropertyContainer* const cont, RS_Graphic* graphic, LC_UCSList* const ucsList,
                                                          int currentUCSIndex, const bool isUCS) const {
    auto clickHandler = [this, currentUCSIndex,ucsList]([[maybe_unused]] RS_Graphic* action, const int linkIndex) {
        switch (linkIndex) {
            case 0: {
                m_actionContext->setCurrentAction(RS2::ActionUCSSetByDimOrdinate, nullptr);
                break;
            }
            case 1: {
                // rename ucs
                const auto existingUCS = ucsList->at(currentUCSIndex);
                if (existingUCS != nullptr) {
                    QC_ApplicationWindow::getAppWindow()->getUCSListWidget()->renameExistingUCS(existingUCS);
                }
                break;
            }
            default:
                break;
        }
    };
    createSingleEntityCommand<RS_Graphic>(cont, "ucsOps", tr("By ordinate dimension"),
                                          tr("Set ucs that corresponds to selected ordinate dimension"),
                                          isUCS ? tr("Rename UCS...") : "",
                                          isUCS ? tr("Renames UCS (ones with empty names are temporary and are not saved)") : "",
                                          graphic, clickHandler, tr("UCS-related commands"));

    // todo - 1) "by ordinate" - may be be valid if there will be quick check for dimensions existence
    // todo - 2) it might be also practical to add support of "ordinates rebase" to current UCS
}

void LC_PropertiesProviderGraphicUCS::createUcsOriginAndDirection(LC_PropertyContainer* const cont, const LC_UCS* const currentUcs) const {
    const auto formatter = m_actionContext->getFormatter();
    const QString strVector = formatter->formatUCSVector(currentUcs->getOrigin());
    const QString originString = QString("[%1]").arg(strVector);

    const LC_Property::Names namesOrigin = {"ucsOrigin", tr("Origin"), tr("Point of coordinates system origin (0,0) in world coordinates")};
    createDirectDelegatedReadonlyString(cont, namesOrigin, originString);

    const double xAxisDirection = currentUcs->getXAxisDirection();
    const QString axisValue = formatter->formatRawAngle(xAxisDirection);

    const LC_Property::Names namesDirection = {
        "ucsXAxis",
        tr("X Axis direction"),
        tr("Direction of X axis of coordinate system in world coordinates")
    };
    createDirectDelegatedReadonlyString(cont, namesDirection, axisValue);
}
