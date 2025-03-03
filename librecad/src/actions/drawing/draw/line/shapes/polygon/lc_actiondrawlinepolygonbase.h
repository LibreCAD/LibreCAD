#ifndef LC_ACTIONDRAWLINEPOLYGONBASE_H
#define LC_ACTIONDRAWLINEPOLYGONBASE_H

#include "rs_previewactioninterface.h"

class LC_ActionDrawLinePolygonBase:public RS_PreviewActionInterface
{
public:
    LC_ActionDrawLinePolygonBase(const char *name, RS_EntityContainer &container, RS_GraphicView &graphicView, RS2::ActionType actionType);
    ~LC_ActionDrawLinePolygonBase() override;

    int getNumber() const{return number;}
    void setNumber(int n) {number = n;}
protected:
    /** Number of edges. */
    int number = 0;
    LC_ActionOptionsWidget* createOptionsWidget() override;
    bool parseNumber(const QString &c);
    RS2::CursorType doGetMouseCursor(int status) override;
};

#endif // LC_ACTIONDRAWLINEPOLYGONBASE_H
