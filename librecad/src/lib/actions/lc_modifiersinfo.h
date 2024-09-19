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

#ifndef LC_MODIFIERSINFO_H
#define LC_MODIFIERSINFO_H


#include <QString>

#define MOD_NONE LC_ModifiersInfo::NONE()
#define MOD_SHIFT_LC(msg) LC_ModifiersInfo::SHIFT(msg)
#define MOD_CTRL(msg) LC_ModifiersInfo::CTRL(msg)
#define MOD_SHIFT_AND_CTRL(shiftMsg, ctrlMsg) LC_ModifiersInfo::SHIFT_AND_CTRL(shiftMsg,ctrlMsg)
#define MOD_SHIFT_AND_CTRL_ANGLE(ctrlMsg) LC_ModifiersInfo::SHIFT_AND_CTRL(LC_ModifiersInfo::MSG_ANGLE_SNAP,ctrlMsg)
#define MOD_SHIFT_ANGLE_SNAP LC_ModifiersInfo::SHIFT_ANGLE_SNAP()
#define MOD_SHIFT_FREE_SNAP LC_ModifiersInfo::SHIFT_FREE_SNAP()
#define MOD_SHIFT_MIRROR_ANGLE LC_ModifiersInfo::SHIFT_MIRROR_ANGLE()
#define MOD_SHIFT_RELATIVE_ZERO LC_ModifiersInfo::SHIFT_RELATIVE_ZERO()

class LC_ModifiersInfo{
public:
    LC_ModifiersInfo();

    static const QString MSG_ANGLE_SNAP;
    static const QString MSG_FREE_SNAP;
    static const QString MSG_MIRROR_ANGLE;
    static const QString MSG_REL_ZERO;
    static const QString MSG_NONE;

    static LC_ModifiersInfo NONE(){return {};};
    static LC_ModifiersInfo SHIFT_ANGLE_SNAP();
    static LC_ModifiersInfo SHIFT_FREE_SNAP();
    static LC_ModifiersInfo SHIFT_MIRROR_ANGLE();
    static LC_ModifiersInfo SHIFT_RELATIVE_ZERO();
    static LC_ModifiersInfo CTRL(const QString &msg);


    static LC_ModifiersInfo SHIFT(const QString &msg);
    static LC_ModifiersInfo SHIFT_AND_CTRL(const QString &shiftMsg, const QString &ctrlMsg);
    const QString&  getShiftMessage() const;
    const QString& getCtrlMessage() const;
protected:
    Qt::KeyboardModifiers modifiers = Qt::NoModifier;
    QString shiftMsg;
    QString ctrlMsg;


    void setFlag(Qt::KeyboardModifier flag){
        modifiers = modifiers | flag;
    }
};

#endif // LC_MODIFIERSINFO_H
