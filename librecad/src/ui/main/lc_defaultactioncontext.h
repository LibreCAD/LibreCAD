#ifndef LC_DEFAULTACTIONCONTEXT_H
#define LC_DEFAULTACTIONCONTEXT_H
#include "lc_actioncontext.h"
#include "qg_actionhandler.h"
#include "rs_document.h"
class QG_CommandWidget;
class QG_CoordinateWidget;
class LC_ActionOptionsManager;
class LC_DefaultActionContext: public LC_ActionContext{
public:
    LC_DefaultActionContext();
    void addOptionsWidget(LC_ActionOptionsWidget *widet) override;
    void removeOptionsWidget(LC_ActionOptionsWidget *widet) override;
    void requestSnapDistOptions(double *dist, bool on) override;
    void requestSnapMiddleOptions(int *middlePoints, bool on) override;
    void hideSnapOptions() override;
    void clearMouseWidgetIcon() override;
    void updateSelectionWidget(int countSelected, double selectedLength) override;
    void updateMouseWidget(const QString &, const QString &, const LC_ModifiersInfo &modifiers) override;
    void commandMessage(const QString &message) override;
    void commandPrompt(const QString &message) override;
    void updateCoordinateWidget(const RS_Vector &abs, const RS_Vector &rel, bool updateFormat) override;
    RS_EntityContainer * getEntityContainer() override;
    RS_GraphicView * getGraphicView() override;
    void setDocumentAndView(RS_Document * document, RS_GraphicView * view);
private:
    LC_ActionOptionsManager* m_actionOptionsManager {nullptr};
    RS_EntityContainer * m_entityContainer {nullptr};
    RS_GraphicView * m_graphicView {nullptr};
    QG_CoordinateWidget* coordinateWidget{nullptr};
    QG_CommandWidget* commandWidget = nullptr;
};

#endif // LC_DEFAULTACTIONCONTEXT_H
