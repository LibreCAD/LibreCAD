#ifndef LC_LINEOPTIONS_H
#define LC_LINEOPTIONS_H

#include<memory>
#include<QWidget>
#include <lc_actiondrawlinesnake.h>

//class RS_ActionInterface;

//class RS_ActionDrawLine;
namespace Ui {
class Ui_LineOptionsRel;
}

class LC_LineOptions : public LC_ActionOptionsWidget
{
    Q_OBJECT

public:
    LC_LineOptions(QWidget* parent = 0, Qt::WindowFlags fl = {});
    ~LC_LineOptions();

public slots:
    virtual void closeLine();
    virtual void undo();
    virtual void redo();
    virtual void polyline();
    virtual void start();
protected:
    LC_ActionDrawLineSnake* action;
    void doSetAction(RS_ActionInterface *a, bool update) override;
protected slots:
    void onAngleClicked(bool value);
    void onXClicked(bool value);
    void onYClicked(bool value);
    void onPointClicked(bool value);
    void onSetAngle();
    void onAngleRelativeClicked(bool value);
    void languageChange() override;
    bool checkActionRttiValid(RS2::ActionType actionType) override;
    QString getSettingsOptionNamePrefix() override;
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
