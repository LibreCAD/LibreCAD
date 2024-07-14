#ifndef LC_LINEOPTIONS_H
#define LC_LINEOPTIONS_H

#include <lc_actiondrawlinesnake.h>
#include "lc_actionoptionswidgetbase.h"

//class RS_ActionInterface;

//class RS_ActionDrawLine;
namespace Ui {
class Ui_LineOptionsRel;
}

class LC_LineOptions : public LC_ActionOptionsWidgetBase{
    Q_OBJECT

public:
    LC_LineOptions();
    ~LC_LineOptions() override;

public slots:
    virtual void closeLine();
    virtual void undo();
    virtual void redo();
    virtual void polyline();
    virtual void start();
protected slots:
    void onAngleClicked(bool value);
    void onXClicked(bool value);
    void onYClicked(bool value);
    void onPointClicked(bool value);
    void onSetAngle();
    void onAngleRelativeClicked(bool value);
    void languageChange() override;
protected:
    LC_ActionDrawLineSnake* action;
    void doSetAction(RS_ActionInterface *a, bool update) override;
    void doSaveSettings() override;
private:
    Ui::Ui_LineOptionsRel* ui;
    bool inUpdateCycle = false;
    void setXDirectionToActionAndView(bool value);
    void setYDirectionToActionAndView(bool value);
    void setAngleDirectionToActionAndView(bool value);
    void setPointDirectionToActionAndView(bool value);
    void setAngleToActionAndView(const QString& val, bool affectState);
    void setAngleRelativeToActionAndView(bool relative);
    void setupAngleRelatedUI(bool value);
};

#endif // LC_LINEOPTIONS_H
