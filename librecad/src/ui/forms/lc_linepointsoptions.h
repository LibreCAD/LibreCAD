#ifndef LC_LINEPOINTSOPTIONS_H
#define LC_LINEPOINTSOPTIONS_H

#include <QWidget>
#include "lc_actionoptionswidget.h"
#include "lc_actiondrawlinepoints.h"

namespace Ui {
class LC_LinePointsOptions;
}

class LC_LinePointsOptions : public LC_ActionOptionsWidget
{
    Q_OBJECT

public:
    explicit LC_LinePointsOptions(QWidget *parent = nullptr);
    ~LC_LinePointsOptions();
protected:
    void saveSettings() override;
    void languageChange() override;
    bool checkActionRttiValid(RS2::ActionType actionType) override;
    void doSetAction(RS_ActionInterface *a, bool update) override;
    void clearAction() override;

protected slots:
    void onPointsCountEditingFinished();
    void onEdgePointsModeIndexChanged(int index);

private:
    Ui::LC_LinePointsOptions *ui;
    LC_ActionDrawLinePoints* action;

    void setPointsCountActionAndView(QString value);
    void setEdgePointsModeToActionAndView(int index);
};

#endif // LC_LINEPOINTSOPTIONS_H
