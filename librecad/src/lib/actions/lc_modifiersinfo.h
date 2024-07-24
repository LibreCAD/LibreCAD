#ifndef LC_MODIFIERSINFO_H
#define LC_MODIFIERSINFO_H

#include <QInternal>

#define MOD_NONE LC_ModifiersInfo::NONE()
#define MOD_SHIFT(msg) LC_ModifiersInfo::SHIFT(msg)
#define MOD_CTRL(msg) LC_ModifiersInfo::CTRL(msg)
#define MOD_SHIFT_AND_CTRL(shiftMsg, ctrlMsg) LC_ModifiersInfo::SHIFT_AND_CTRL(shiftMsg,ctrlMsg)
#define MOD_SHIFT_AND_CTRL_ANGLE(ctrlMsg) LC_ModifiersInfo::SHIFT_AND_CTRL(LC_ModifiersInfo::MSG_ANGLE_SNAP,ctrlMsg)
#define MOD_SHIFT_ANGLE_SNAP LC_ModifiersInfo::SHIFT_ANGLE_SNAP()
#define MOD_SHIFT_FREE_SNAP LC_ModifiersInfo::SHIFT_FREE_SNAP()
#define MOD_SHIFT_MIRROR_ANGLE LC_ModifiersInfo::SHIFT_MIRROR_ANGLE()
#define MOD_SHIFT_RELATIVE_ZERO LC_ModifiersInfo::SHIFT_RELATIVE_ZERO()

class LC_ModifiersInfo
{
public:
    LC_ModifiersInfo();

    static const char* MSG_ANGLE_SNAP;
    static const char* MSG_FREE_SNAP;
    static const char* MSG_MIRROR_ANGLE;
    static const char* MSG_REL_ZERO;

    static LC_ModifiersInfo NONE(){return LC_ModifiersInfo();};
    static LC_ModifiersInfo SHIFT_ANGLE_SNAP();
    static LC_ModifiersInfo SHIFT_FREE_SNAP();
    static LC_ModifiersInfo SHIFT_MIRROR_ANGLE();
    static LC_ModifiersInfo SHIFT_RELATIVE_ZERO();
    static LC_ModifiersInfo CTRL(const char* msg);


    static LC_ModifiersInfo SHIFT(const char* msg);
    static LC_ModifiersInfo SHIFT_AND_CTRL(const char* shiftMsg, const char* ctrlMsg);
    const char* getShiftMessage() const;
    const char* getCtrlMessage() const;
protected:
    Qt::KeyboardModifiers modifiers = Qt::NoModifier;
    const char* shiftMsg = nullptr;
    const char* ctrlMsg = nullptr;


    void setFlag(Qt::KeyboardModifier flag){
        modifiers = modifiers | flag;
    }
};

#endif // LC_MODIFIERSINFO_H
