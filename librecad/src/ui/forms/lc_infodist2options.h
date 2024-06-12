#ifndef LC_INFODIST2OPTIONS_H
#define LC_INFODIST2OPTIONS_H

#include <QWidget>
#include "lc_actionoptionswidget.h"
#include "rs_actioninfodist2.h"

namespace Ui {
class LC_InfoDist2Options;
}

class LC_InfoDist2Options : public LC_ActionOptionsWidget
{
    Q_OBJECT

public:
    explicit LC_InfoDist2Options(QWidget *parent = nullptr);
    ~LC_InfoDist2Options() override;

protected slots:
    void languageChange() override;
    void onOnEntityClicked(bool value);

protected:
    void doSaveSettings() override;
    void doSetAction( RS_ActionInterface * a, bool update) override;
    bool checkActionRttiValid(RS2::ActionType actionType) override;
    QString getSettingsOptionNamePrefix() override;

private:
    Ui::LC_InfoDist2Options *ui;
    RS_ActionInfoDist2* action = nullptr;
    void setOnEntitySnapToActionAndView(bool value);
};

#endif // LC_INFODIST2OPTIONS_H
