#include "lc_defaultactioncontext.h"

#include "lc_actionoptionsmanager.h"
#include "qg_commandwidget.h"
#include "qg_coordinatewidget.h"
#include "qg_selectionwidget.h"
LC_DefaultActionContext::LC_DefaultActionContext() {}

void LC_DefaultActionContext::addOptionsWidget(LC_ActionOptionsWidget *widet){
    m_actionOptionsManager->addOptionsWidget(widet);
}

void LC_DefaultActionContext::removeOptionsWidget(LC_ActionOptionsWidget *widet){
    m_actionOptionsManager->removeOptionsWidget(widet);
}

void LC_DefaultActionContext::requestSnapDistOptions(double *dist, bool on){
    m_actionOptionsManager->requestSnapDistOptions(dist, on);
}

void LC_DefaultActionContext::requestSnapMiddleOptions(int *middlePoints, bool on){
    m_actionOptionsManager->requestSnapMiddleOptions(middlePoints, on);
}

void LC_DefaultActionContext::hideSnapOptions(){
    m_actionOptionsManager->hideSnapOptions();
}

void LC_DefaultActionContext::clearMouseWidgetIcon(){

}

void LC_DefaultActionContext::updateSelectionWidget(int countSelected, double selectedLength){
    if (selectionWidget != nullptr) {
        selectionWidget->setNumber(countSelected);
        selectionWidget->setTotalLength(selectedLength);
    }
}

void LC_DefaultActionContext::updateMouseWidget(const QString &chars, const QString &string, const LC_ModifiersInfo &modifiers){

}

void LC_DefaultActionContext::commandMessage(const QString &message){
    if (commandWidget) {
        commandWidget->appendHistory(message);
    }
}

void LC_DefaultActionContext::commandPrompt(const QString &message){
    if (commandWidget) {
        commandWidget->setCommand(message);
    }
}

void LC_DefaultActionContext::updateCoordinateWidget(const RS_Vector &abs, const RS_Vector &rel, bool updateFormat){
    if (coordinateWidget != nullptr) {
        coordinateWidget->setCoordinates(abs, rel, updateFormat);
    }
}

RS_EntityContainer * LC_DefaultActionContext::getEntityContainer(){
    return m_entityContainer;
}

RS_GraphicView * LC_DefaultActionContext::getGraphicView(){
    return m_graphicView;
}

void LC_DefaultActionContext::setDocumentAndView(RS_Document *document, RS_GraphicView *view){
    m_graphicView = view;
    m_entityContainer = document;

}
