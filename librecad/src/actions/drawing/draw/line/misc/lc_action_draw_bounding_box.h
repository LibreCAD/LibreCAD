#ifndef LC_ACTIONDRAWBOUNDINGBOX_H
#define LC_ACTIONDRAWBOUNDINGBOX_H


#include "lc_actionpreselectionawarebase.h"

class RS_Pen;
class RS_Layer;

class LC_ActionDrawBoundingBox : public LC_ActionPreSelectionAwareBase{
    Q_OBJECT
public:
    explicit LC_ActionDrawBoundingBox(LC_ActionContext *actionContext);
    bool isSelectionAsGroup() const{return m_selectionAsGroup;}
    void setSelectionAsGroup(const bool val){m_selectionAsGroup = val;}
    bool isCornerPointsOnly() const {return m_cornerPointsOnly;}
    void setCornersOnly(const bool val){m_cornerPointsOnly = val;}
    bool isCreatePolyline() const {return m_createPolyline;}
    void setCreatePolyline(const bool val){m_createPolyline = val;}
    void setOffset(const double o){m_offset = o;}
    double getOffset() const{return m_offset;}
    void init(int status) override;
protected:
    bool m_selectionAsGroup = false;
    bool m_cornerPointsOnly = false;
    bool m_createPolyline = false;
    double m_offset = 0.0;

    void createPoint(double x, double y,QList<RS_Entity*> &entitiesList ) const;
    void createLine(double x1, double y1, double x2, double y2,QList<RS_Entity*> &entitiesList ) const;
    void createCornerPoints(const RS_Vector &selectionMin, const RS_Vector &selectionMax,QList<RS_Entity*> &entitiesList ) const;
    void createBoxLines(const RS_Vector &selectionMin, const RS_Vector &selectionMax,QList<RS_Entity*> &entitiesList ) const;
    void createBoxPolyline(const RS_Vector &selectionMin, const RS_Vector &selectionMax,QList<RS_Entity*> &entitiesList ) const;
    LC_ActionOptionsWidget *createOptionsWidget() override;
    LC_ActionOptionsPropertiesFiller* createOptionsFiller() override;
    void updateActionPromptForSelection() override;
    bool isAllowTriggerOnEmptySelection() override;
    bool doUpdateDistanceByInteractiveInput(const QString& tag, double distance) override;
    void doTriggerCompletion(bool success) override;
    void doTriggerSelectionUpdate(bool keepSelected, const LC_DocumentModificationBatch& ctx) override;
    bool doTriggerModifications(LC_DocumentModificationBatch& ctx) override;
    void doSaveOptions() override;
    void doLoadOptions() override;
};

#endif
