/****************************************************************************
**
* Abstract base class form modification actions

Copyright (C) 2024 LibreCAD.org
Copyright (C) 2024 sand1024

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
**********************************************************************/
#include "lc_actionmodifybase.h"

LC_ActionModifyBase::LC_ActionModifyBase(const char *name, RS_EntityContainer &container, RS_GraphicView &graphicView,
    const QList<RS2::EntityType> &entityTypeList, bool countSelectionDeep)
    :LC_ActionPreSelectionAwareBase(name, container, graphicView, entityTypeList, countSelectionDeep){}

void LC_ActionModifyBase::selectionCompleted([[maybe_unused]] bool singleEntity, bool fromInit) {
    setSelectionComplete(isAllowTriggerOnEmptySelection(), fromInit);
    updateMouseButtonHints();
    updateSelectionWidget();
}

#define SHOW_MODIFICATION_DIALOGS  false;

bool LC_ActionModifyBase::isShowModifyActionDialog() {
    return SHOW_MODIFICATION_DIALOGS; // fixme - add options support? or just remove dialogs later?
}

void LC_ActionModifyBase::setUseCurrentLayer(bool b) {
    LC_ModifyOperationFlags* data = getModifyOperationFlags();
    data->useCurrentLayer = b;
}

bool LC_ActionModifyBase::isUseCurrentLayer() {
    LC_ModifyOperationFlags* data = getModifyOperationFlags();
    return data->useCurrentLayer;
}

void LC_ActionModifyBase::setUseCurrentAttributes(bool b) {
    LC_ModifyOperationFlags* data = getModifyOperationFlags();
    data->useCurrentAttributes = b;
}

bool LC_ActionModifyBase::isUseCurrentAttributes() {
    LC_ModifyOperationFlags* data = getModifyOperationFlags();
    return data->useCurrentAttributes;
}

int LC_ActionModifyBase::getCopiesNumber() {
    LC_ModifyOperationFlags* data = getModifyOperationFlags();
    return data->number;
}

void LC_ActionModifyBase::setCopiesNumber(int value) {
    LC_ModifyOperationFlags* data = getModifyOperationFlags();
    data->number = value;
}

void LC_ActionModifyBase::setKeepOriginals(bool b) {
    LC_ModifyOperationFlags* data = getModifyOperationFlags();
    data->keepOriginals = b;
}

void LC_ActionModifyBase::setUseMultipleCopies(bool val) {
    LC_ModifyOperationFlags* data = getModifyOperationFlags();
    data->multipleCopies = val;
}

bool LC_ActionModifyBase::isUseMultipleCopies() {
    LC_ModifyOperationFlags* data = getModifyOperationFlags();
    return data->multipleCopies;
}

bool LC_ActionModifyBase::isKeepOriginals() {
    LC_ModifyOperationFlags* data = getModifyOperationFlags();
    return data->keepOriginals;
}
