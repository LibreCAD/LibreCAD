#ifndef LC_ACTIONDRAWBOUNDINGBOX_H
#define LC_ACTIONDRAWBOUNDINGBOX_H


#include "lc_actionpreselectionawarebase.h"

class RS_Pen;
class RS_Layer;

class LC_ActionDrawBoundingBox : public LC_ActionPreSelectionAwareBase{
    Q_OBJECT
public:
    LC_ActionDrawBoundingBox(LC_ActionContext *actionContext);
    bool isSelectionAsGroup() const{return m_selectionAsGroup;}
    void setSelectionAsGroup(bool val){m_selectionAsGroup = val;}
    bool isCornerPointsOnly() const {return m_cornerPointsOnly;}
    void setCornersOnly(bool val){m_cornerPointsOnly = val;}
    bool isCreatePolyline() const {return m_createPolyline;}
    void setCreatePolyline(bool val){m_createPolyline = val;}
    void setOffset(double o){m_offset = o;};
    double getOffset() const{return m_offset;}
    void init(int status) override;
protected:
    bool m_selectionAsGroup = false;
    bool m_cornerPointsOnly = false;
    bool m_createPolyline = false;
    double m_offset = 0.0;

    void createPoint(RS_Layer *activeLayer, const RS_Pen &pen, double x, double y);
    void createLine(RS_Layer *activeLayer, const RS_Pen &pen, double x1, double y1, double x2, double y2);
    void createCornerPoints(RS_Layer *activeLayer, const RS_Pen &pen, const RS_Vector &selectionMin, const RS_Vector &selectionMax);
    void createBoxLines(RS_Layer *activeLayer, const RS_Pen &pen, const RS_Vector &selectionMin, const RS_Vector &selectionMax);
    void createBoxPolyline(RS_Layer *activeLayer, const RS_Pen &pen, const RS_Vector &selectionMin, const RS_Vector &selectionMax);
    LC_ActionOptionsWidget *createOptionsWidget() override;
    void updateMouseButtonHintsForSelection() override;
    void doTrigger(bool keepSelected) override;
    bool isAllowTriggerOnEmptySelection() override;
    bool doUpdateDistanceByInteractiveInput(const QString& tag, double distance) override;
};

#endif // LC_ACTIONDRAWBOUNDINGBOX_H
