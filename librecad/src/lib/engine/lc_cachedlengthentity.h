#ifndef LC_CACHEDLENGTHENTITY_H
#define LC_CACHEDLENGTHENTITY_H

#include "rs_atomicentity.h"

class LC_CachedLengthEntity:public RS_AtomicEntity{
public:
    LC_CachedLengthEntity(RS_EntityContainer* parent = nullptr);

    double getLength() const override{
        return cachedLength;
    }

protected:

    // cached length for painting speedup
    double cachedLength = 0.0;
    virtual void updateLength() = 0;
};

#endif // LC_CACHEDLENGTHENTITY_H
