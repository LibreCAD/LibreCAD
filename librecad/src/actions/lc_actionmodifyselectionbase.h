#ifndef LC_ACTIONMODIFYSELECTIONBASE_H
#define LC_ACTIONMODIFYSELECTIONBASE_H

#include "rs_previewactioninterface.h"

class LC_ActionModifySelectionBase:public RS_PreviewActionInterface {
Q_OBJECT
public:
    LC_ActionModifySelectionBase(
       const char *name,
       RS_EntityContainer &container,
       RS_GraphicView &graphicView,
       RS2::ActionType actionType = RS2::ActionNone);

    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *e) override;
protected:
    bool selectionFinished = false;
    int countSelected();
};

#endif // LC_ACTIONMODIFYSELECTIONBASE_H
