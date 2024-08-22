/****************************************************************************
**
* Holder for information about keybaard modifiers supported by actions

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
#include "lc_modifiersinfo.h"
#include <QObject>

LC_ModifiersInfo::LC_ModifiersInfo() {}

const QString LC_ModifiersInfo::MSG_ANGLE_SNAP =  QObject::tr("Angle Snap");
const QString LC_ModifiersInfo::MSG_FREE_SNAP =  QObject::tr("Free Snap");
const QString LC_ModifiersInfo::MSG_MIRROR_ANGLE = QObject::tr("Use Mirrored Angle");
const QString LC_ModifiersInfo::MSG_REL_ZERO = QObject::tr("Snap to Relative Zero");
const QString LC_ModifiersInfo::MSG_NONE = "";

LC_ModifiersInfo LC_ModifiersInfo::SHIFT_ANGLE_SNAP(){
    return SHIFT(MSG_ANGLE_SNAP);
}

LC_ModifiersInfo LC_ModifiersInfo::SHIFT_FREE_SNAP(){
    return SHIFT(MSG_FREE_SNAP);
}

LC_ModifiersInfo LC_ModifiersInfo::SHIFT_MIRROR_ANGLE(){
    return SHIFT(MSG_MIRROR_ANGLE);
}

LC_ModifiersInfo LC_ModifiersInfo::SHIFT_RELATIVE_ZERO(){
    return SHIFT(MSG_REL_ZERO);
}

LC_ModifiersInfo LC_ModifiersInfo::SHIFT(const QString &msg){
    LC_ModifiersInfo result = LC_ModifiersInfo();
    result.setFlag(Qt::ShiftModifier);
    result.shiftMsg = msg;
    return result;
}

LC_ModifiersInfo LC_ModifiersInfo::CTRL(const QString &msg){
    LC_ModifiersInfo result = LC_ModifiersInfo();
    result.setFlag(Qt::ControlModifier);
    result.ctrlMsg = msg;
    return result;
}

LC_ModifiersInfo LC_ModifiersInfo::SHIFT_AND_CTRL(const QString &shiftMsg, const QString &ctrlMsg){
    LC_ModifiersInfo result = LC_ModifiersInfo();
    result.setFlag(Qt::ControlModifier);
    result.setFlag(Qt::ShiftModifier);
    result.ctrlMsg = ctrlMsg;
    result.shiftMsg = shiftMsg;
    return result;
}

const QString& LC_ModifiersInfo::getShiftMessage() const{
    if (modifiers & Qt::ShiftModifier){
        return shiftMsg;
    }
    else {
        return MSG_NONE;
    }
}

const QString& LC_ModifiersInfo::getCtrlMessage() const{
    if (modifiers & Qt::ControlModifier){
        return ctrlMsg;
    }
    else {
        return MSG_NONE;
    }
}
