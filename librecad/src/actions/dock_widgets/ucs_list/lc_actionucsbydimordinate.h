#ifndef LC_ACTIONUCSBYDIMORDINATE_H
#define LC_ACTIONUCSBYDIMORDINATE_H

#include "lc_action_select_single_entity_base.h"

class LC_DimOrdinate;

class LC_ActionUCSByDimOrdinate: public LC_ActionSingleEntitySelectBase{
    Q_OBJECT
public:
    explicit LC_ActionUCSByDimOrdinate(LC_ActionContext* actionContext);
    ~LC_ActionUCSByDimOrdinate() override;
protected:
    void doTrigger() override;
    bool doCheckMaySelectEntity(RS_Entity* e) override;
    QString doGetMouseButtonHint() override;
};

#endif
