#include "lc_modifiersinfo.h"

LC_ModifiersInfo::LC_ModifiersInfo() {}

const char* LC_ModifiersInfo::MSG_ANGLE_SNAP = "Angle Snap";
const char* LC_ModifiersInfo::MSG_FREE_SNAP = "Free Snap";
const char* LC_ModifiersInfo::MSG_MIRROR_ANGLE = "Use Mirrorred Angle";
const char* LC_ModifiersInfo::MSG_REL_ZERO = "Snap to Relative Zero";

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

LC_ModifiersInfo LC_ModifiersInfo::SHIFT(const char* msg){
    LC_ModifiersInfo result = LC_ModifiersInfo();
    result.setFlag(Qt::ShiftModifier);
    result.shiftMsg = msg;
    return result;
}

LC_ModifiersInfo LC_ModifiersInfo::CTRL(const char* msg){
    LC_ModifiersInfo result = LC_ModifiersInfo();
    result.setFlag(Qt::ControlModifier);
    result.ctrlMsg = msg;
    return result;
}

LC_ModifiersInfo LC_ModifiersInfo::SHIFT_AND_CTRL(const char* shiftMsg, const char* ctrlMsg){
    LC_ModifiersInfo result = LC_ModifiersInfo();
    result.setFlag(Qt::ControlModifier);
    result.setFlag(Qt::ShiftModifier);
    result.ctrlMsg = ctrlMsg;
    result.shiftMsg = shiftMsg;
    return result;
}

const char* LC_ModifiersInfo::getShiftMessage() const{
    if (modifiers & Qt::ShiftModifier){
        return shiftMsg;
    }
    else {
        return nullptr;
    }
}

const char* LC_ModifiersInfo::getCtrlMessage() const{
    if (modifiers & Qt::ControlModifier){
        return ctrlMsg;
    }
    else {
        return nullptr;
    }
}
