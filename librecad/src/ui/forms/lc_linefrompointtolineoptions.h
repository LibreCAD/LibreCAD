#ifndef LC_LINEFROMPOINTTOLINEOPTIONS_H
#define LC_LINEFROMPOINTTOLINEOPTIONS_H

#include <QWidget>
#include "lc_actionoptionswidget.h"
#include "lc_actiondrawlinefrompointtoline.h"

namespace Ui {
class LC_LineFromPointToLineOptions;
}
/**
 * UI options for LC_ActionDrawLineFromPointToLine
 */
class LC_LineFromPointToLineOptions :public LC_ActionOptionsWidget
{
    Q_OBJECT
protected:
    void doSetAction(RS_ActionInterface *a, bool update) override;
    void clearAction() override;
    bool checkActionRttiValid(RS2::ActionType actionType) override;

protected slots:
    void onAngleEditingFinished();
    void onLengthEditingFinished();
    void onSnapModeIndexChanged(int index);
    void onSizeModeIndexChanged(int index);
    void onOrthogonalClicked(bool value);
    void saveSettings() override;
    void languageChange() override;
public:
    explicit LC_LineFromPointToLineOptions(QWidget *parent = nullptr);
    ~LC_LineFromPointToLineOptions() override;

private:
    LC_ActionDrawLineFromPointToLine* action;
    Ui::LC_LineFromPointToLineOptions *ui;
    void setOrthogonalToActionAndView(bool value);
    void setSizeModelIndexToActionAndView(int index);
    void setSnapModeToActionAndView(int index);
    void setAngleToActionAndView(QString value);
    void setLengthToActionAndView(QString value);
};

#endif // LC_LINEFROMPOINTTOLINEOPTIONS_H
