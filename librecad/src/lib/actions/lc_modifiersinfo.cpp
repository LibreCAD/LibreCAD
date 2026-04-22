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

LC_ModifiersInfo::LC_ModifiersInfo() = default;

const QString LC_ModifiersInfo::MSG_EMPTY = "";

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
    auto result = LC_ModifiersInfo();
    result.setFlag(Qt::ShiftModifier);
    result.m_shiftMsg = msg;
    return result;
}

LC_ModifiersInfo LC_ModifiersInfo::CTRL(const QString &msg){
    auto result = LC_ModifiersInfo();
    result.setFlag(Qt::ControlModifier);
    result.m_ctrlMsg = msg;
    return result;
}

LC_ModifiersInfo LC_ModifiersInfo::SHIFT_AND_CTRL(const QString &shiftMsg, const QString &ctrlMsg){
    auto result = LC_ModifiersInfo();
    result.setFlag(Qt::ControlModifier);
    result.setFlag(Qt::ShiftModifier);
    result.m_ctrlMsg = ctrlMsg;
    result.m_shiftMsg = shiftMsg;
    return result;
}

const QString& LC_ModifiersInfo::getShiftMessage() const {
    if ((m_modifiers & Qt::ShiftModifier) != 0u) {
        return m_shiftMsg;
    }
    return MSG_EMPTY;
}

const QString& LC_ModifiersInfo::getCtrlMessage() const{
    if ((m_modifiers & Qt::ControlModifier) != 0u) {
        return m_ctrlMsg;
    }
    return MSG_EMPTY;
}
