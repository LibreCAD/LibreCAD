#ifndef LC_ACTIONMODIFYBASE_H
#define LC_ACTIONMODIFYBASE_H

#include "lc_actionpreselectionawarebase.h"
#include "rs_modification.h"

class LC_ActionModifyBase:public LC_ActionPreSelectionAwareBase{

public:
    LC_ActionModifyBase(
        const char *name, RS_EntityContainer &container, RS_GraphicView &graphicView,
        const QList<RS2::EntityType> &entityTypeList = {}, bool countSelectionDeep = false);

    void setUseCurrentLayer(bool b);
    bool isUseCurrentLayer();
    void setUseCurrentAttributes(bool b);
    bool isUseCurrentAttributes();
    int getCopiesNumber();
    void setCopiesNumber(int value);
    bool isUseMultipleCopies();
    void setUseMultipleCopies(bool val);
    bool isKeepOriginals();
    void setKeepOriginals(bool b);
protected:
    virtual bool isShowModifyActionDialog();
    void selectionCompleted(bool singleEntity) override;
    virtual LC_ModifyOperationFlags* getModifyOperationFlags()=0;
};

#endif // LC_ACTIONMODIFYBASE_H
