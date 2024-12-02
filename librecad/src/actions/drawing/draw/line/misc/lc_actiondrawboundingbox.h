#ifndef LC_ACTIONDRAWBOUNDINGBOX_H
#define LC_ACTIONDRAWBOUNDINGBOX_H

#include <QObject>
#include "rs_previewactioninterface.h"
#include "lc_actionpreselectionawarebase.h"

class LC_ActionDrawBoundingBox : public LC_ActionPreSelectionAwareBase{
    Q_OBJECT
public:
    LC_ActionDrawBoundingBox(RS_EntityContainer &container,RS_GraphicView &graphicView);
    bool isSelectionAsGroup(){return selectionAsGroup;}
    void setSelectionAsGroup(bool val){selectionAsGroup = val;}
    bool isCornerPointsOnly(){return cornerPointsOnly;}
    void setCornersOnly(bool val){cornerPointsOnly = val;}
    bool isCreatePolyline(){return createPolyline;}
    void setCreatePolyline(bool val){createPolyline = val;}
    void init(int status) override;
protected:
    bool selectionAsGroup = false;
    bool cornerPointsOnly = false;
    bool createPolyline = false;

    void createPoint(RS_Layer *activeLayer, const RS_Pen &pen, double x, double y);
    void createLine(RS_Layer *activeLayer, const RS_Pen &pen, double x1, double y1, double x2, double y2);
    void createCornerPoints(RS_Layer *activeLayer, const RS_Pen &pen, const RS_Vector &selectionMin, const RS_Vector &selectionMax);
    void createBoxLines(RS_Layer *activeLayer, const RS_Pen &pen, const RS_Vector &selectionMin, const RS_Vector &selectionMax);
    void createBoxPolyline(RS_Layer *activeLayer, const RS_Pen &pen, const RS_Vector &selectionMin, const RS_Vector &selectionMax);
    LC_ActionOptionsWidget *createOptionsWidget() override;
    void updateMouseButtonHintsForSelection() override;
    void doTrigger(bool keepSelected) override;
    bool isAllowTriggerOnEmptySelection() override;
};

#endif // LC_ACTIONDRAWBOUNDINGBOX_H
