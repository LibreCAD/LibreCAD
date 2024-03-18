#ifndef LC_CIRCLEBYARCOPTIONS_H
#define LC_CIRCLEBYARCOPTIONS_H

#include <QWidget>
#include "lc_actionoptionswidget.h"
#include "lc_actiondrawcirclebyarc.h"

namespace Ui {
class LC_CircleByArcOptions;
}
/**
 * Options for CircleByArc action
 */
class LC_CircleByArcOptions : public LC_ActionOptionsWidget
{
    Q_OBJECT

public:
    explicit LC_CircleByArcOptions(QWidget *parent = nullptr);
    ~LC_CircleByArcOptions();

protected:
    void saveSettings() override;
    void languageChange() override;
    bool checkActionRttiValid(RS2::ActionType actionType) override;
    void doSetAction(RS_ActionInterface *a, bool update) override;
    void clearAction() override;

protected slots:
    void onReplaceClicked(bool value);

private:
    Ui::LC_CircleByArcOptions *ui;
    LC_ActionDrawCircleByArc* action;

    void setReplaceArcToActionAndView(bool value);
};

#endif // LC_CIRCLEBYARCOPTIONS_H
